//
// Created by dshatalin on 10/15/26.
//

#include "parser.hpp"

#include <format>

using namespace hack_decoder;

namespace {
    inline constexpr std::string_view invalid_char_fmt{
        "encountered invalid character: {:c}"
    };
    inline constexpr std::string_view invalid_char_in_target_fmt{
        "encountered invalid character in target [ @target ]: {:c}"
    };
    inline constexpr std::string_view invalid_char_target_label_or_addr_fmt{
        "a-instruction target can only be a label or an address: {:c}"
    };
    inline constexpr std::string_view invalid_char_in_dest_fmt{
        "encountered invalid character in dest [ dest = comp ; jump ]: {:c}"
    };
    inline constexpr std::string_view invalid_char_in_comp_fmt{
        "encountered invalid character in comp [ dest = comp ; jump ]: {:c}"
    };
    inline constexpr std::string_view invalid_char_in_jump_fmt{
        "encountered invalid character in jump [ dest = comp ; jump ]: {:c}"
    };
    inline constexpr std::string_view invalid_char_in_label_fmt{
        "encountered invalid character in label [ (label) ]: {:c}"
    };

    enum class st : uint8_t {
        in_label,
        in_instruction,
        in_comment_block,
        in_error,
        empty
    };

    bool is_space(const unsigned char c) {
        return c == ' ' || c == '\t' || c == '\r';
    }

    bool is_one_of(const unsigned char c, const std::string_view chars) {
        return chars.find(static_cast<char>(c)) != std::string_view::npos;
    }
}

class scanner_base::impl {
    using step_fn = state (impl::*)(unsigned char);

    step_fn step_ = &impl::state_statement_begin;
    st state_{st::empty};
    size_t instruction_count_{0};
    size_t line_count_{0};
    std::optional<diagnostic> diag_{};

public:
    void reset();

    auto feed(std::byte b) -> state;

    auto eof() -> state;

    [[nodiscard]] auto instruction() const -> size_t;

    [[nodiscard]] auto line() const -> size_t;

    [[nodiscard]] auto last_diagnostic() const -> const std::optional<diagnostic> &;

    void reset_diagnostic();

private:
    auto state_statement_begin(unsigned char c) -> state;

    auto state_a_instruction_begin(unsigned char c) -> state;

    auto state_target_label(unsigned char c) -> state;

    auto state_target_address(unsigned char c) -> state;

    auto state_a(unsigned char c) -> state;

    auto state_m(unsigned char c) -> state;

    auto state_d(unsigned char c) -> state;

    auto state_equal(unsigned char c) -> state;

    auto state_comp_negative(unsigned char c) -> state;

    auto state_comp_negate(unsigned char c) -> state;

    auto state_comp_left(unsigned char c) -> state;

    auto state_comp_operator(unsigned char c) -> state;

    auto state_comp_right(unsigned char c) -> state;

    auto state_jump_begin(unsigned char c) -> state;

    auto state_jump_j(unsigned char c) -> state;

    auto state_jump_g_l(unsigned char c) -> state;

    auto state_jump_e(unsigned char c) -> state;

    auto state_jump_n(unsigned char c) -> state;

    auto state_jump_m(unsigned char c) -> state;

    auto state_jump_end(unsigned char c) -> state;

    auto state_label_begin(unsigned char c) -> state;

    auto state_label(unsigned char c) -> state;

    auto state_label_end(unsigned char c) -> state;

    auto state_comment_begin(unsigned char c) -> state;

    auto state_comment_line(unsigned char c) -> state;

    auto state_comment_multiline(unsigned char c) -> state;

    auto state_comment_multiline_end(unsigned char c) -> state;

    auto state_in_error(unsigned char c) -> state;

    void weak_warn(std::string msg);

    void warn(std::string msg);

    auto fail(std::string msg) -> state;

    auto go(step_fn next, state emit) -> state;

    void set_state(st expected, st next);

    void reset_state(st expected);
};

// scanner_base

scanner_base::scanner_base()
    : pimpl(std::make_unique<impl>()) {
}

scanner_base::~scanner_base() = default;

scanner_base::scanner_base(scanner_base &&) noexcept = default;

scanner_base &scanner_base::operator=(scanner_base &&) noexcept = default;

void scanner_base::reset() const {
    this->pimpl->reset();
}

auto scanner_base::feed(const std::byte c) const -> state {
    return this->pimpl->feed(c);
}

auto scanner_base::eof() const -> state {
    return this->pimpl->eof();
}

void scanner_base::reset_diagnostic() const {
    this->pimpl->reset_diagnostic();
}

auto scanner_base::instruction() const -> size_t {
    return this->pimpl->instruction();
}

auto scanner_base::line() const -> size_t {
    return this->pimpl->line();
}

auto scanner_base::last_diagnostic() const -> const std::optional<diagnostic> & {
    return this->pimpl->last_diagnostic();
}

// scanner_base::impl

void scanner_base::impl::reset() {
    this->step_ = &impl::state_statement_begin;
    this->state_ = st::empty;
    this->instruction_count_ = 0;
    this->line_count_ = 1;
    this->diag_.reset();
}

auto scanner_base::impl::feed(const std::byte b) -> state {
    const auto c = std::to_integer<unsigned char>(b);
    const auto result = (this->*step_)(c);
    if (result == state::scan_end_instruction) ++this->instruction_count_;
    if (c == '\n') ++this->line_count_;
    return result;
}

auto scanner_base::impl::eof() -> state {
    switch (this->state_) {
        case st::in_label:
            return this->fail("unexpected end for label");
        case st::in_instruction:
            return this->fail("unexpected end for instruction");
        case st::in_comment_block:
            return this->fail("unexpected end for comment block");
        case st::in_error:
            return state::scan_error;
        default:
            if (this->step_ != &impl::state_statement_begin) {
                return this->fail("unexpected end of file");
            }
    }
    return state::scan_end;
}

void scanner_base::impl::reset_diagnostic() {
    this->diag_.reset();
}

auto scanner_base::impl::instruction() const -> size_t {
    return this->instruction_count_;
}

auto scanner_base::impl::line() const -> size_t {
    return this->line_count_;
}

auto scanner_base::impl::last_diagnostic() const -> const std::optional<diagnostic> & {
    return this->diag_;
}

auto scanner_base::impl::state_statement_begin(const unsigned char c) -> state {
    if (is_space(c)) {
        return state::scan_skip_space;
    }
    switch (c) {
        case '@':
            return this->go(&impl::state_a_instruction_begin, state::scan_begin_a_instruction);
        case 'M':
            return this->go(&impl::state_m, state::scan_begin_c_instruction);
        case 'D':
            return this->go(&impl::state_d, state::scan_begin_c_instruction);
        case 'A':
            return this->go(&impl::state_a, state::scan_begin_c_instruction);
        case '0':
            this->weak_warn("this can be omitted");
            return this->go(&impl::state_comp_right, state::scan_begin_c_instruction);
        case 'J':
            return this->go(&impl::state_jump_j, state::scan_begin_c_instruction);
        case '(':
            return this->go(&impl::state_label_begin, state::scan_begin_label);
        case '/':
            return this->go(&impl::state_comment_begin, state::scan_skip_space);
        case '\n':
            return state::scan_skip_space;
        default:
            return this->fail(std::format(invalid_char_fmt, c));
    }
}

auto scanner_base::impl::state_a_instruction_begin(const unsigned char c) -> state {
    this->state_ = st::in_instruction;
    if (isalpha(c) || c == '_' || c == '-') {
        return this->go(&impl::state_target_label, state::scan_continue);
    }
    if (isdigit(c)) {
        return this->go(&impl::state_target_address, state::scan_continue);
    }
    return this->fail(std::format(invalid_char_target_label_or_addr_fmt, c));
}

auto scanner_base::impl::state_target_label(const unsigned char c) -> state {
    if (isalnum(c) || c == '_' || c == '-') {
        return state::scan_continue;
    }
    if (is_space(c) || c == '\n') {
        return this->go(&impl::state_statement_begin, state::scan_end_instruction);
    }
    if (c == '/') {
        return this->go(&impl::state_comment_begin, state::scan_end_instruction);
    }
    return this->fail(std::format(invalid_char_in_target_fmt, c));
}

auto scanner_base::impl::state_target_address(const unsigned char c) -> state {
    if (isdigit(c)) {
        return state::scan_continue;
    }
    if (is_space(c) || c == '\n') {
        return this->go(&impl::state_statement_begin, state::scan_end_instruction);
    }
    if (c == '/') {
        return this->go(&impl::state_comment_begin, state::scan_end_instruction);
    }
    return this->fail(std::format(invalid_char_in_target_fmt, c));
}

auto scanner_base::impl::state_a(const unsigned char c) -> state {
    this->set_state(st::empty, st::in_instruction);
    if (is_space(c)) {
        return state::scan_skip_space;
    }
    switch (c) {
        case 'D':
            return this->go(&impl::state_d, state::scan_continue);
        case 'M':
            return this->go(&impl::state_m, state::scan_continue);
        case '=':
            return this->go(&impl::state_equal, state::scan_continue);
        case ';':
            this->warn("JMP relies on A register to perform goto; this can lead to undefined behavior");
            return this->go(&impl::state_jump_begin, state::scan_continue);
        default:
            return this->fail(std::format(invalid_char_in_dest_fmt, c));
    }
}

auto scanner_base::impl::state_m(const unsigned char c) -> state {
    this->set_state(st::empty, st::in_instruction);
    if (is_space(c)) {
        return state::scan_skip_space;
    }
    if (c == 'D') {
        return this->go(&impl::state_d, state::scan_continue);
    }
    if (c == '=') {
        return this->go(&impl::state_equal, state::scan_continue);
    }
    if (c == ';') {
        this->warn("JMP relies on A register to perform goto; this can lead to undefined behavior");
        return this->go(&impl::state_jump_begin, state::scan_continue);
    }
    return this->fail(std::format(invalid_char_in_dest_fmt, c));
}

auto scanner_base::impl::state_d(const unsigned char c) -> state {
    this->set_state(st::empty, st::in_instruction);
    if (is_space(c)) {
        return state::scan_skip_space;
    }
    if (c == '=') {
        return this->go(&impl::state_equal, state::scan_continue);
    }
    if (c == ';') {
        return this->go(&impl::state_jump_begin, state::scan_continue);
    }
    return this->fail(std::format(invalid_char_in_dest_fmt, c));
}

auto scanner_base::impl::state_equal(const unsigned char c) -> state {
    if (is_space(c)) {
        return state::scan_skip_space;
    }
    if (is_one_of(c, "01")) {
        return this->go(&impl::state_comp_right, state::scan_continue);
    }
    if (c == '-') {
        return this->go(&impl::state_comp_negative, state::scan_continue);
    }
    if (c == '!') {
        return this->go(&impl::state_comp_negate, state::scan_continue);
    }
    if (is_one_of(c, "AMD")) {
        return this->go(&impl::state_comp_left, state::scan_continue);
    }
    return this->fail(std::format(invalid_char_in_comp_fmt, c));
}

auto scanner_base::impl::state_comp_negative(const unsigned char c) -> state {
    if (is_one_of(c, "1AMD")) {
        return this->go(&impl::state_comp_right, state::scan_continue);
    }
    return this->fail(std::format(invalid_char_in_comp_fmt, c));
}

auto scanner_base::impl::state_comp_negate(const unsigned char c) -> state {
    if (is_one_of(c, "AMD")) {
        return this->go(&impl::state_comp_right, state::scan_continue);
    }
    return this->fail(std::format(invalid_char_in_comp_fmt, c));
}

auto scanner_base::impl::state_comp_left(const unsigned char c) -> state {
    if (is_space(c)) {
        return state::scan_skip_space;
    }
    if (is_one_of(c, "+-&|")) {
        return this->go(&impl::state_comp_operator, state::scan_continue);
    }
    if (c == '\n') {
        return this->go(&impl::state_statement_begin, state::scan_end_instruction);
    }
    if (c == ';') {
        return this->go(&impl::state_jump_begin, state::scan_continue);
    }
    return this->fail(std::format(invalid_char_in_comp_fmt, c));
}

auto scanner_base::impl::state_comp_operator(const unsigned char c) -> state {
    if (is_space(c)) {
        return state::scan_skip_space;
    }
    if (is_one_of(c, "1ADM")) {
        return this->go(&impl::state_comp_right, state::scan_continue);
    }
    return this->fail(std::format(invalid_char_in_comp_fmt, c));
}

auto scanner_base::impl::state_comp_right(const unsigned char c) -> state {
    this->set_state(st::empty, st::in_instruction);
    if (is_space(c)) {
        return state::scan_skip_space;
    }
    switch (c) {
        case '\n':
            return this->go(&impl::state_statement_begin, state::scan_end_instruction);
        case ';':
            return this->go(&impl::state_jump_begin, state::scan_continue);
        default:
            return this->fail(std::format(invalid_char_in_comp_fmt, c));
    }
}

auto scanner_base::impl::state_jump_begin(const unsigned char c) -> state {
    if (is_space(c)) {
        return state::scan_skip_space;
    }
    if (c == 'J') {
        return this->go(&impl::state_jump_j, state::scan_continue);
    }
    return this->fail(std::format(invalid_char_in_jump_fmt, c));
}

auto scanner_base::impl::state_jump_j(const unsigned char c) -> state {
    this->set_state(st::empty, st::in_instruction);
    switch (c) {
        case 'G':
        case 'L':
            return this->go(&impl::state_jump_g_l, state::scan_continue);
        case 'E':
            return this->go(&impl::state_jump_e, state::scan_continue);
        case 'N':
            return this->go(&impl::state_jump_n, state::scan_continue);
        case 'M':
            return this->go(&impl::state_jump_m, state::scan_continue);
        default:
            return this->fail(std::format(invalid_char_in_jump_fmt, c));
    }
}

auto scanner_base::impl::state_jump_g_l(const unsigned char c) -> state {
    switch (c) {
        case 'T':
        case 'E':
            return this->go(&impl::state_jump_end, state::scan_continue);
        default:
            return this->fail(std::format(invalid_char_in_jump_fmt, c));
    }
}

auto scanner_base::impl::state_jump_e(const unsigned char c) -> state {
    if (c == 'Q') {
        return this->go(&impl::state_jump_end, state::scan_continue);
    }
    return this->fail(std::format(invalid_char_in_jump_fmt, c));
}

auto scanner_base::impl::state_jump_n(const unsigned char c) -> state {
    if (c == 'E') {
        return this->go(&impl::state_jump_end, state::scan_continue);
    }
    return this->fail(std::format(invalid_char_in_jump_fmt, c));
}

auto scanner_base::impl::state_jump_m(const unsigned char c) -> state {
    if (c == 'P') {
        return this->go(&impl::state_jump_end, state::scan_continue);
    }
    return this->fail(std::format(invalid_char_in_jump_fmt, c));
}

auto scanner_base::impl::state_jump_end(const unsigned char c) -> state {
    this->reset_state(st::in_instruction);
    if (is_space(c)) {
        return this->go(&impl::state_statement_begin, state::scan_end_instruction);
    }
    if (c == '\n') {
        return this->go(&impl::state_statement_begin, state::scan_end_instruction);
    }
    if (c == '/') {
        return this->go(&impl::state_comment_begin, state::scan_end_instruction);
    }
    return this->fail(std::format(invalid_char_in_jump_fmt, c));
}

auto scanner_base::impl::state_label_begin(const unsigned char c) -> state {
    this->state_ = st::in_label;
    if (isalnum(c) || c == '_' || c == '-') {
        return this->go(&impl::state_label, state::scan_continue);
    }
    if (c == ')') {
        return this->fail("label cannot be empty");
    }
    return this->fail(std::format(invalid_char_in_label_fmt, c));
}

auto scanner_base::impl::state_label(const unsigned char c) -> state {
    if (isalnum(c) || c == '_' || c == '-') {
        return state::scan_continue;
    }
    if (c == ')') {
        return this->go(&impl::state_label_end, state::scan_continue);
    }
    return this->fail(std::format(invalid_char_in_label_fmt, c));
}

auto scanner_base::impl::state_label_end(const unsigned char c) -> state {
    this->reset_state(st::in_label);
    if (is_space(c) || c == '\n') {
        return this->go(&impl::state_statement_begin, state::scan_end_label);
    }
    if (c == '/') {
        return this->go(&impl::state_comment_begin, state::scan_end_label);
    }
    return this->fail(std::format(invalid_char_in_label_fmt, c));
}

auto scanner_base::impl::state_comment_begin(const unsigned char c) -> state {
    if (c == '/') {
        return this->go(&impl::state_comment_line, state::scan_skip_space);
    }
    if (c == '*') {
        this->state_ = st::in_comment_block;
        return this->go(&impl::state_comment_multiline, state::scan_skip_space);
    }
    return this->fail(std::format(invalid_char_fmt, c));
}

auto scanner_base::impl::state_comment_line(const unsigned char c) -> state {
    if (c == '\n') {
        return this->go(&impl::state_statement_begin, state::scan_skip_space);
    }
    return state::scan_skip_space;
}

auto scanner_base::impl::state_comment_multiline(const unsigned char c) -> state {
    if (c == '*') {
        return this->go(&impl::state_comment_multiline_end, state::scan_skip_space);
    }
    return state::scan_skip_space;
}

auto scanner_base::impl::state_comment_multiline_end(const unsigned char c) -> state {
    if (c == '/') {
        this->reset_state(st::in_comment_block);
        return this->go(&impl::state_statement_begin, state::scan_skip_space);
    }
    return this->go(&impl::state_comment_multiline, state::scan_skip_space);
}

auto scanner_base::impl::state_in_error(unsigned char c) -> state { // NOLINT(*-convert-member-functions-to-static)
    return state::scan_error;
}

void scanner_base::impl::weak_warn(std::string msg) {
    this->diag_.emplace(severity::weak_warning, this->line(), std::move(msg));
}

void scanner_base::impl::warn(std::string msg) {
    this->diag_.emplace(severity::warning, this->line(), std::move(msg));
}

auto scanner_base::impl::fail(std::string msg) -> state {
    this->diag_.emplace(severity::error, this->line(), std::move(msg));
    this->step_ = &impl::state_in_error;
    this->state_ = st::in_error;
    return state::scan_error;
}

auto scanner_base::impl::go(const step_fn next, const state emit) -> state {
    this->step_ = next;
    return emit;
}

void scanner_base::impl::set_state(const st expected, const st next) {
    if (this->state_ == expected) this->state_ = next;
}

void scanner_base::impl::reset_state(const st expected) {
    if (this->state_ == expected) this->state_ = st::empty;
}
