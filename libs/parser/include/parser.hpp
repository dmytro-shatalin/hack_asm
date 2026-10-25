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


namespace hack_decoder {
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

    class scanner_base {
        class impl;

        std::unique_ptr<impl> pimpl;

    public:
        // Constructors
        scanner_base();

        ~scanner_base();

        scanner_base(scanner_base &) = delete;

        scanner_base(scanner_base &&) noexcept;

        // Operators
        scanner_base &operator=(scanner_base &) = delete;

        scanner_base &operator=(scanner_base &&) noexcept;

    protected:
        void reset() const;

        [[nodiscard]] auto feed(std::byte c) const -> state;

        [[nodiscard]] auto eof() const -> state;

        [[nodiscard]] auto instruction() const -> size_t;

        [[nodiscard]] auto line() const -> size_t;

        [[nodiscard]] auto last_diagnostic() const -> const std::optional<diagnostic> &;

        void reset_diagnostic() const;
    };

    class scanner : scanner_base {
    public:
        void reset() const {
            scanner_base::reset();
        }

        [[nodiscard]] auto feed(const std::byte c) const -> state {
            return scanner_base::feed(c);
        }

        [[nodiscard]] auto eof() const -> state {
            return scanner_base::eof();
        }

        [[nodiscard]] auto instruction() const -> size_t {
            return scanner_base::instruction();
        }

        [[nodiscard]] auto line() const -> size_t {
            return scanner_base::line();
        }

        [[nodiscard]] auto last_diagnostic() const -> std::optional<diagnostic> {
            return scanner_base::last_diagnostic();
        }

        void reset_diagnostic() const {
            scanner_base::reset_diagnostic();
        }
    };

    class parser_base {
        class impl;
        std::unique_ptr<impl> pimpl;

    public:
        // Constructors
        explicit parser_base(std::vector<std::byte> buffer);

        ~parser_base();

        parser_base(parser_base &) = delete;

        parser_base(parser_base &&) noexcept;

        // Operators
        parser_base &operator=(parser_base &) = delete;

        parser_base &operator=(parser_base &&) noexcept;

    protected:
        void reset() const;

        [[nodiscard]] auto parse() const -> std::expected<std::vector<instruction_t>, diagnostic>;

        [[nodiscard]] auto warnings() const -> std::span<const diagnostic>;
    };

    class parser : parser_base {
    public:
        explicit parser(std::vector<std::byte> buffer)
            : parser_base(std::move(buffer)) {
        }

        void reset() const {
            parser_base::reset();
        }

        [[nodiscard]] auto parse() const -> std::expected<std::vector<instruction_t>, diagnostic> {
            return parser_base::parse();
        }

        [[nodiscard]] auto warnings() const -> std::span<const diagnostic> {
            return parser_base::warnings();
        }
    };
}


#endif //HACK_ASM_DECODER_H
