//
// Created by dshatalin on 10/15/26.
//

#include "parser.hpp"

using namespace hack_decoder;

class parser_base::impl {
    std::vector<std::byte> buffer_;
    scanner scan_{};
    state opcode_{state::scan_continue};
    size_t off_{0};
    std::unordered_map<std::string, size_t> labels_;
    std::vector<diagnostic> warnings_;

public:
    explicit impl(std::vector<std::byte> buffer);

    impl(const impl &) = delete;

    impl(impl &&) noexcept = default;

    impl &operator=(const impl &) = delete;

    impl &operator=(impl &&) = default;

    void reset();

    auto parse() -> std::expected<std::vector<statement_t>, diagnostic>;

    auto warnings() const -> std::span<const diagnostic>;

private:
    auto read_offset() const -> size_t { return this->off_; };

    auto parse_label() -> std::optional<diagnostic>;

    auto check_valid() -> std::optional<diagnostic>;

    auto scan_next() -> state;

    auto scan_while(state expected) -> size_t;

    auto scan_label() -> size_t;

    auto scan_instruction() -> size_t;

    bool eof();
};

// parse_base

parser_base::parser_base(std::vector<std::byte> buffer)
    : pimpl(std::make_unique<impl>(std::move(buffer))) {
}

parser_base::~parser_base() = default;

parser_base::parser_base(parser_base &&) noexcept = default;

parser_base &parser_base::operator =(parser_base &&) noexcept = default;

void parser_base::reset() const {
    this->pimpl->reset();
}

auto parser_base::parse() const -> std::expected<std::vector<statement_t>, diagnostic> {
    return this->pimpl->parse();
}

auto parser_base::warnings() const -> std::span<const diagnostic> {
    return this->pimpl->warnings();
}

// parsed_base::impl

parser_base::impl::impl(std::vector<std::byte> buffer)
    : buffer_(std::move(buffer)) {
}

void parser_base::impl::reset() {
    this->labels_.clear();
    this->off_ = 0;
    this->scan_.reset();
    this->warnings_.clear();
}

auto parser_base::impl::scan_next() -> state {
    if (this->eof()) return state::scan_end;
    this->opcode_ = this->scan_.feed(this->buffer_[this->off_++]);
    return this->opcode_;
}

auto parser_base::impl::scan_while(const state expected) -> size_t {
    while (this->scan_next() != expected) {
    }
    return this->off_;
}

auto parser_base::impl::scan_label() -> size_t {
    if (this->opcode_ != state::scan_begin_label) return this->off_;
    while (this->scan_next() != state::scan_end_label) {
    }
    return this->off_;
}

auto parser_base::impl::scan_instruction() -> size_t {
    if (this->opcode_ != state::scan_begin_a_instruction &&
        this->opcode_ != state::scan_begin_c_instruction)
        return this->off_;
    while (this->scan_next() != state::scan_end_instruction) {
    }
    return this->off_;
}

bool parser_base::impl::eof() {
    if (this->off_ >= this->buffer_.size()) {
        this->opcode_ = state::scan_end;
        return true;
    }
    return false;
}

static auto to_string(auto first, auto last) -> std::string {
    return std::ranges::to<std::string>(
        std::ranges::subrange(first, last) |
        std::views::transform([](std::byte b) { return static_cast<char>(b); }));
}

auto parser_base::impl::parse_label() -> std::optional<diagnostic> {
    const auto line = this->scan_.line();
    const auto instr = this->scan_.instruction();

    const auto it_start = this->buffer_.begin() + static_cast<long>(this->read_offset() + 1);
    const auto it_end = this->buffer_.begin() + static_cast<long>(this->scan_label());

    const auto label = to_string(it_start, it_end);

    if (label.empty()) {
        return diagnostic{.level = severity::error, .line = line, .msg = "label cannot be empty"};
    }

    if (this->labels_.contains(label)) {
        return diagnostic{.level = severity::error, .line = line, .msg = "label already defined"};
    }

    this->labels_[label] = instr;
    return std::nullopt;
}

auto parser_base::impl::check_valid() -> std::optional<diagnostic> {
    std::optional<diagnostic> error{};

    while (this->scan_next() != state::scan_end) {
        if (this->opcode_ == state::scan_error) {
            break;
        }
        if (this->opcode_ == state::scan_begin_label) {
            if (const auto parse_err = this->parse_label(); parse_err.has_value()) {
                error.emplace(parse_err.value());
                break;
            }
        }
        if (const auto diag = this->scan_.last_diagnostic(); diag.has_value()) {
            this->warnings_.push_back(diag.value());
            this->scan_.reset_diagnostic();
        }
    }

    if (this->scan_.eof() != state::scan_error) return std::nullopt;

    auto diag = this->scan_.last_diagnostic();
    if (!diag.has_value()) {
        diag.emplace(diagnostic{.level = severity::error, .line = 0, .msg = "unknown error"});
    }
    this->reset();
    return diag.value();
}

auto parser_base::impl::parse() -> std::expected<std::vector<statement_t>, diagnostic> {
    this->reset();

    if (const auto error = this->check_valid(); error.has_value()) {
        return std::unexpected(error.value());
    }

    std::vector<statement_t> statements;
    // todo: add parsing logic

    return statements;
}

auto parser_base::impl::warnings() const -> std::span<const diagnostic> {
    return this->warnings_;
}
