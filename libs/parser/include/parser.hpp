//
// Created by dshatalin on 10/15/26.
//

#ifndef HACK_ASM_DECODER_H
#define HACK_ASM_DECODER_H

#include <cstdint>
#include <expected>
#include <functional>
#include <memory>
#include <optional>
#include <stack>
#include <string>
#include <utility>
#include <vector>


namespace hack_asm {
    enum class state : uint8_t {
        scan_continue,
        scan_begin_a_instruction,
        scan_begin_c_instruction,
        scan_end_instruction,
        scan_begin_label,
        scan_end_label,
        scan_skip_space,
        scan_end,
        scan_error,
    };

    enum class severity : uint8_t { weak_warning, warning, error };

    struct diagnostic {
        severity level;
        std::size_t line;
        std::string msg;
    };

    struct a_instruction {
        std::uint16_t target;
    };

    struct c_instruction {
        std::uint8_t dest;
        std::uint8_t comp;
        std::uint8_t jump;
    };

    using instruction_t = std::variant<a_instruction, c_instruction>;

    class scanner {
        class impl;

        std::unique_ptr<impl> pimpl;

    public:
        // Constructors
        scanner();

        ~scanner();

        scanner(scanner &) = delete;

        scanner(scanner &&) noexcept;

        // Operators
        scanner &operator=(scanner &) = delete;

        scanner &operator=(scanner &&) noexcept;

        void reset() const;

        [[nodiscard]] auto feed(std::byte c) const -> state;

        [[nodiscard]] auto eof() const -> state;

        [[nodiscard]] auto instruction() const -> size_t;

        [[nodiscard]] auto line() const -> size_t;

        [[nodiscard]] auto last_diagnostic() const -> const std::optional<diagnostic> &;

        void reset_diagnostic() const;
    };

    class parser {
        class impl;
        std::unique_ptr<impl> pimpl;

    public:
        // Constructors
        explicit parser(std::vector<std::byte> buffer);

        ~parser();

        parser(parser &) = delete;

        parser(parser &&) noexcept;

        // Operators
        parser &operator=(parser &) = delete;

        parser &operator=(parser &&) noexcept;

        void reset() const;

        [[nodiscard]] auto parse() const -> std::expected<std::vector<instruction_t>, diagnostic>;

        [[nodiscard]] auto warnings() const -> std::span<const diagnostic>;
    };
}


#endif //HACK_ASM_DECODER_H
