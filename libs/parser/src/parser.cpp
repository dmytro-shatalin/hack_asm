//
// Created by dshatalin on 10/15/26.
//

#include "parser.hpp"

#include <format>

using namespace hack_asm;

namespace {
    const std::unordered_map<std::string, uint8_t> comp_lut{
        {"0", 0b0101010}, {"1", 0b0111111}, {"-1", 0b0111010},
        {"D", 0b0001100}, {"A", 0b0110000}, {"M", 0b1110000},
        {"!D", 0b0001101}, {"!A", 0b0110001}, {"!M", 0b1110001},
        {"-D", 0b0001111}, {"-A", 0b0110011}, {"-M", 0b1110011},
        {"D+1", 0b0011111}, {"A+1", 0b0110111}, {"M+1", 0b1110111},
        {"D-1", 0b0001110}, {"A-1", 0b0110010}, {"M-1", 0b1110010},
        {"D+A", 0b0000010}, {"D+M", 0b1000010}, {"D-A", 0b0010011},
        {"D-M", 0b1010011}, {"A-D", 0b0000111}, {"M-D", 0b1000111},
        {"D&A", 0b0000000}, {"D&M", 0b1000000},
        {"D|A", 0b0010101}, {"D|M", 0b1010101},
    };

    const std::unordered_map<std::string, uint8_t> jump_lut{
        {"null", 0b000},
        {"JGT", 0b001},
        {"JEQ", 0b010},
        {"JGE", 0b011},
        {"JLT", 0b100},
        {"JNE", 0b101},
        {"JLE", 0b110},
        {"JMP", 0b111},
    };

    auto to_string(auto first, auto last) -> std::string {
        return std::ranges::to<std::string>(
            std::ranges::subrange(first, last) |
            std::views::transform([](std::byte b) { return static_cast<char>(b); }));
    }

    auto parse_a_instruction(const std::string &str, std::unordered_map<std::string, size_t> &labels,
                             const size_t line, instruction_t &out) -> std::optional<diagnostic> {
        size_t addr;
        if (auto target = std::string{str}.substr(1); std::isalpha(target[0]) || target[0] == '_') {
            if (!labels.contains(target)) {
                return diagnostic{
                    .level = severity::error, .line = line, .msg = std::format("undefined label: {:s}", target)
                };
            }
            addr = labels[target];
        } else {
            std::uint16_t value{};
            const auto [ptr, ec] = std::from_chars(target.data(), target.data() + target.size(), value);
            if (ec != std::errc{} || ptr != target.data() + target.size() || value > 32767) {
                return diagnostic{
                    .level = severity::error, .line = line, .msg = std::format("invalid address: {}", target)
                };
            }
            addr = value;
        }
        out = a_instruction(addr);
        return std::nullopt;
    }

    auto parse_c_instruction(const std::string &str, const size_t line,
                             instruction_t &out) -> std::optional<diagnostic> {
        std::string_view rest = str;
        std::string_view dest, jump;

        if (rest == "JMP") {
            out = c_instruction{.dest = 0, .comp = comp_lut.at("0"), .jump = jump_lut.at("JMP")};
            return std::nullopt;
        }
        if (const auto eq = rest.find('='); eq != std::string_view::npos) {
            dest = rest.substr(0, eq);
            rest.remove_prefix(eq + 1);
        }
        if (const auto sc = rest.find(';'); sc != std::string_view::npos) {
            jump = rest.substr(sc + 1);
            rest = rest.substr(0, sc);
        }
        const std::string_view comp = rest;

        c_instruction instr{};
        for (const char c: dest) {
            switch (c) {
                case 'A': instr.dest |= 0b100;
                    break;
                case 'D': instr.dest |= 0b010;
                    break;
                case 'M': instr.dest |= 0b001;
                    break;
                default:
                    return diagnostic{
                        .level = severity::error, .line = line, .msg = std::format("invalid dest: {}", dest)
                    };
            }
        }
        const auto c_it = comp_lut.find(std::string{comp});
        if (c_it == comp_lut.end()) {
            return diagnostic{
                .level = severity::error, .line = line, .msg = std::format("invalid comp: '{}'", comp)
            };
        }
        instr.comp = c_it->second;

        if (!jump.empty()) {
            const auto j_it = jump_lut.find(std::string{jump});
            if (j_it == jump_lut.end()) {
                return diagnostic{
                    .level = severity::error, .line = line, .msg = std::format("invalid jump: '{}'", jump)
                };
            }
            instr.jump = j_it->second;
        }
        out = instr;
        return std::nullopt;
    }
}

class parser::impl {
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

    auto skip_while(state expected) -> state;

    auto scan_until(state target) -> size_t;

    auto scan_label() -> size_t;

    auto scan_instruction() -> size_t;

    bool eof();
};

// parse_base

parser::parser(std::vector<std::byte> buffer)
    : pimpl(std::make_unique<impl>(std::move(buffer))) {
}

parser::~parser() = default;

parser::parser(parser &&) noexcept = default;

parser &parser::operator =(parser &&) noexcept = default;

void parser::reset() const {
    this->pimpl->reset();
}

auto parser::parse() const -> std::expected<std::vector<instruction_t>, diagnostic> {
    return this->pimpl->parse();
}

auto parser::warnings() const -> std::span<const diagnostic> {
    return this->pimpl->warnings();
}

// parsed_base::impl

parser::impl::impl(std::vector<std::byte> buffer)
    : buffer_(std::move(buffer)) {
}

void parser::impl::reset() {
    this->labels_.clear();
    this->off_ = 0;
    this->scan_.reset();
    this->warnings_.clear();
}

auto parser::impl::scan_next() -> state {
    if (this->eof()) return state::scan_end;
    this->opcode_ = this->scan_.feed(this->buffer_[this->off_++]);
    return this->opcode_;
}

auto parser::impl::skip_while(const state expected) -> state {
    state s;
    while ((s = this->scan_next()) == expected) {
    }
    return s;
}

auto parser::impl::scan_until(const state target) -> size_t {
    state s;
    do {
        s = this->scan_next();
    } while (s != target && s != state::scan_end && s != state::scan_error);
    return this->off_;
}

auto parser::impl::scan_label() -> size_t {
    if (this->opcode_ != state::scan_begin_label) return this->off_;
    return this->scan_until(state::scan_end_label);
}

auto parser::impl::scan_instruction() -> size_t {
    if (this->opcode_ != state::scan_begin_a_instruction &&
        this->opcode_ != state::scan_begin_c_instruction)
        return this->off_;
    return this->scan_until(state::scan_end_instruction);
}

bool parser::impl::eof() {
    if (this->off_ >= this->buffer_.size()) {
        this->opcode_ = state::scan_end;
        return true;
    }
    return false;
}

auto parser::impl::parse_label() -> std::optional<diagnostic> {
    const auto line = this->scan_.line();
    const auto instr = this->scan_.instruction();

    const auto first = this->read_offset();
    const auto last = this->scan_label();
    if (this->opcode_ == state::scan_error) {
        return this->scan_.last_diagnostic();
    }

    // scan_end_label is emitted on the character after ')'; at EOF there is none
    const long trailing = this->opcode_ == state::scan_end_label ? 2 : 1;
    const auto it_start = this->buffer_.begin() + static_cast<long>(first);
    const auto it_end = this->buffer_.begin() + static_cast<long>(last) - trailing;

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

auto parser::impl::parse_instruction(instruction_t &out) -> std::optional<diagnostic> {
    const auto line = this->scan_.line();
    const auto opcode = this->opcode_;

    // the begin token was emitted for the first character, which is already consumed
    const auto first = this->read_offset() - 1;
    const auto last = this->scan_instruction();

    // scan_end_instruction is emitted on the character after the instruction; at EOF there is none
    const long trailing = this->opcode_ == state::scan_end_instruction ? 1 : 0;
    const auto it_start = this->buffer_.begin() + static_cast<long>(first);
    const auto it_end = this->buffer_.begin() + static_cast<long>(last) - trailing;

    auto str = to_string(it_start, it_end);
    for (const char c: " \t\r\n") {
        std::erase(str, c);
    }
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

auto parser::impl::check_valid() -> std::optional<diagnostic> {
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

    if (this->scan_.eof() != state::scan_error) {
        this->off_ = 0;
        this->scan_.reset();
        return std::nullopt;
    }

    auto diag = this->scan_.last_diagnostic();
    if (!diag.has_value()) {
        diag.emplace(diagnostic{.level = severity::error, .line = 0, .msg = "unknown error"});
    }
    return diag.value();
}

auto parser::impl::parse() -> std::expected<std::vector<instruction_t>, diagnostic> {
    this->reset();

    if (const auto error = this->check_valid(); error.has_value()) {
        return std::unexpected(error.value());
    }

    state opcode = this->skip_while(state::scan_skip_space);

    std::vector<instruction_t> statements;
    while (opcode != state::scan_end) {
        if (opcode == state::scan_begin_a_instruction || opcode == state::scan_begin_c_instruction) {
            instruction_t parsed;
            if (const auto parse_err = this->parse_instruction(parsed); parse_err.has_value()) {
                return std::unexpected{parse_err.value()};
            }
            statements.push_back(parsed);
        }
        opcode = this->scan_next();
    }

    return statements;
}

auto parser::impl::warnings() const -> std::span<const diagnostic> {
    return this->warnings_;
}
