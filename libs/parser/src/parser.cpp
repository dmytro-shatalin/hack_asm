//
// Created by dshatalin on 10/15/26.
//

#include "parser.hpp"

#include <format>
#include <regex>

using namespace hack_decoder;

namespace {
    auto to_string(auto first, auto last) -> std::string {
        return std::ranges::to<std::string>(
            std::ranges::subrange(first, last) |
            std::views::transform([](std::byte b) { return static_cast<char>(b); }));
    }

    auto parse_a_instruction(const std::string &str, std::unordered_map<std::string, size_t> &labels,
                             const size_t line, instruction_t &out) -> std::optional<diagnostic> {
        static const std::regex a_regex{
            R"(@((?:[A-Za-z_.$:][A-Za-z0-9_.$:]*)|(?:[0-9]+)))"
        };

        std::smatch m;
        if (!std::regex_search(str, m, a_regex)) {
            return diagnostic{.level = severity::error, .line = line, .msg = "invalid instruction"};
        }

        if (!m[1].matched) {
            return diagnostic{.level = severity::error, .line = line, .msg = "invalid label"};
        }

        size_t addr;
        if (auto target = m[1].str(); std::isalpha(target[0])) {
            if (!labels.contains(target)) {
                return diagnostic{
                    .level = severity::error, .line = line, .msg = std::format("undefined label: {:s}", target)
                };
            }
            addr = labels[target];
        } else {
            addr = std::stoi(target);
        }
        out = a_instruction(addr);
        return std::nullopt;
    }

    auto parse_c_instruction(const std::string &str, const size_t line,
                             instruction_t &out) -> std::optional<diagnostic> {
        static const std::regex c_regex{
            R"((?:(?:([AMD]{1,3})=)?((?:0|1|-1)|(?:[!-]?[DAM])|(?:[DAM][-+]1)|(?:D[-+&|][AM])|(?:[AM]-D))(?:;(JGT|JEQ|JGE|JLT|JNE|JLE|JMP))?)|(JMP))"
        };

        static const std::unordered_map<std::string, size_t> comp_lut{
            {"0", 0b0101010},
            {"1", 0b0111111},
            {"-1", 0b0111010},
            {"D", 0b0001100},
            {"A", 0b0110000},
            {"M", 0b1110011},
            {"!D", 0b0001101},
            {"!A", 0b0110001},
            {"!M", 0b1110001},
            {"-D", 0b0001111},
            {"-A", 0b0110011},
            {"-M", 0b1110011},
            {"D+1", 0b0011111},
            {"A+1", 0b0110111},
            {"M+1", 0b1110111},
            {"D-1", 0b0001110},
            {"A-1", 0b0110111},
            {"M-1", 0b1110111},
            {"D+A", 0b0000010},
            {"D+M", 0b1000010},
            {"D-A", 0b0010011},
            {"D-M", 0b1010011},
            {"A-D", 0b0000111},
            {"M-D", 0b1000111},
            {"D&A", 0b0000000},
            {"D&M", 0b1000000},
            {"D|A", 0b0010101},
            {"D|M", 0b1010101},
        };

        static const std::unordered_map<std::string, size_t> jump_lut{
            {"null", 0b000},
            {"JGT", 0b001},
            {"JEQ", 0b010},
            {"JGE", 0b011},
            {"JLT", 0b100},
            {"JNE", 0b101},
            {"JLE", 0b110},
            {"JMP", 0b111},
        };

        std::smatch m;
        if (!std::regex_match(str, m, c_regex)) {
            return diagnostic{.level = severity::error, .line = line, .msg = "invalid instruction"};
        }

        c_instruction instr{};
        if (m[4].matched) {
            instr.comp = comp_lut.at("0");
            instr.jump = jump_lut.at("JMP");
        } else {
            if (m[1].matched) {
                for (const auto dest = m[1].str(); const char c: dest) {
                    switch (c) {
                        case 'A': instr.dest |= 0b100;
                            break;
                        case 'D': instr.dest |= 0b010;
                            break;
                        case 'M': instr.dest |= 0b001;
                            break;
                        default:
                            return diagnostic{.level = severity::error, .line = line, .msg = "invalid destination"};
                    }
                }
            }
            if (m[2].matched) {
                instr.comp = comp_lut.at(m[2].str());
            }
            if (m[3].matched) {
                instr.jump = jump_lut.at(m[3].str());
            }
        }
        out = instr;
        return std::nullopt;
    }
}

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

    auto parse() -> std::expected<std::vector<instruction_t>, diagnostic>;

    auto warnings() const -> std::span<const diagnostic>;

private:
    auto read_offset() const -> size_t { return this->off_; };

    auto parse_label() -> std::optional<diagnostic>;

    auto parse_instruction(instruction_t &out) -> std::optional<diagnostic>;

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

auto parser_base::parse() const -> std::expected<std::vector<instruction_t>, diagnostic> {
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

auto parser_base::impl::parse_instruction(instruction_t &out) -> std::optional<diagnostic> {
    const auto line = this->scan_.line();
    const auto opcode = this->opcode_;

    const auto it_start = this->buffer_.begin() + static_cast<long>(this->read_offset());
    const auto it_end = this->buffer_.begin() + static_cast<long>(this->scan_instruction());

    auto str = to_string(it_start, it_end);
    std::erase(str, ' ');
    switch (opcode) {
        case state::scan_begin_a_instruction: {
            return parse_a_instruction(str, this->labels_, line, out);
        }
        case state::scan_begin_c_instruction: {
            return parse_c_instruction(str, line, out);
        }
        default:
            return std::nullopt;
    }
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

    if (error.has_value()) {
        return error.value();
    }

    if (this->scan_.eof() != state::scan_error) return std::nullopt;

    auto diag = this->scan_.last_diagnostic();
    if (!diag.has_value()) {
        diag.emplace(diagnostic{.level = severity::error, .line = 0, .msg = "unknown error"});
    }
    this->reset();
    return diag.value();
}

auto parser_base::impl::parse() -> std::expected<std::vector<instruction_t>, diagnostic> {
    this->reset();

    if (const auto error = this->check_valid(); error.has_value()) {
        return std::unexpected(error.value());
    }

    this->scan_while(state::scan_skip_space);

    state opcode;
    std::vector<instruction_t> statements;
    while ((opcode = this->scan_next()) != state::scan_end) {
        if (opcode != state::scan_begin_a_instruction && opcode != state::scan_begin_c_instruction) {
            continue;
        }
        instruction_t parsed;
        if (const auto parse_err = this->parse_instruction(parsed); parse_err.has_value()) {
            return std::unexpected{parse_err.value()};
        }
        statements.push_back(parsed);
    }

    return statements;
}

auto parser_base::impl::warnings() const -> std::span<const diagnostic> {
    return this->warnings_;
}
