#include <cstddef>
#include <atomic>
#include <cstring>
#include <cstdint>
#include "vendor.hxx"
#include "tty.hxx"
#include "locale.hxx"

extern "C"
{
    struct ubsan_source
    {
        const char *file;
        uint32_t line;
        uint32_t col;
    };

    enum ubsan_type_kind
    {
        UBSAN_KIND_INT = 0x000,
        UBSAN_KIND_FLOAT = 0x0001,
        UBSAN_KIND_UNKNOWN = 0xFFFF
    };

    struct ubsan_type
    {
        uint16_t kind;
        uint16_t info;
        char name[0];
    };

    struct ubsan_overflow
    {
        ubsan_source loc;
        const ubsan_type *type;
    };

    struct ubsan_invalid_value
    {
        ubsan_source loc;
        const ubsan_type *type;
    };

    struct ubsan_type_mismatch
    {
        ubsan_source loc;
        const ubsan_type *type;
        unsigned char align;
        unsigned char check_kind;
    };

    struct ubsan_shift_oob
    {
        ubsan_source loc;
        const ubsan_type *left_type;
        const ubsan_type *right_type;
    };

    struct ubsan_array_oob
    {
        ubsan_source loc;
        const ubsan_type *array_type;
        const ubsan_type *index_type;
    };

    struct ubsan_unreachable
    {
        ubsan_source loc;
    };

    struct ubsan_invalid_builtin
    {
        ubsan_source loc;
        unsigned char kind;
    };

    struct ubsan_non_null_return
    {
        ubsan_source loc;
    };

    struct ubsan_non_null_argument
    {
        ubsan_source loc;
    };

    struct ubsan_negative_vla
    {
        ubsan_source loc;
        const ubsan_type *type;
    };

    std::atomic_bool ubsan_lock = false;

    static UBSAN_FUNC void ubsan_print_location(const char *msg, ubsan_source *loc)
    {
        if (ubsan_lock)
            return;

        ubsan_lock = true;
        if (loc != nullptr && loc->file != nullptr)
        {
            char filebuf[24];
            for (size_t i = 0; i < std::strlen(loc->file) && i < sizeof(filebuf); i++)
                filebuf[i] = Locale::Convert<Locale::Charset::ASCII, Locale::Charset::NATIVE>(loc->file[i]);
            TTY::Print("ubsan: %s @ %s:%i:%i\n", msg, filebuf, (int)loc->line, (int)loc->col);
        }
        else
        {
            TTY::Print("ubsan: %s @ (corrupt location %p)\n", msg, loc);
        }
        ubsan_lock = false;
    }

    extern "C" UBSAN_FUNC void __ubsan_handle_add_overflow(ubsan_overflow *data)
    {
        ubsan_print_location("addition overflow", &data->loc);
    }

    extern "C" UBSAN_FUNC void __ubsan_handle_sub_overflow(ubsan_overflow *data)
    {
        ubsan_print_location("subtraction overflow", &data->loc);
    }

    extern "C" UBSAN_FUNC void __ubsan_handle_mul_overflow(ubsan_overflow *data)
    {
        ubsan_print_location("multiply overflow", &data->loc);
    }

    extern "C" UBSAN_FUNC void __ubsan_handle_divrem_overflow(ubsan_overflow *data)
    {
        ubsan_print_location("division/remainder overflow", &data->loc);
    }

    extern "C" UBSAN_FUNC void __ubsan_handle_negate_overflow(ubsan_overflow *data)
    {
        ubsan_print_location("negate overflow", &data->loc);
    }

    extern "C" UBSAN_FUNC void __ubsan_handle_pointer_overflow(ubsan_overflow *data)
    {
        ubsan_print_location("pointer overflow", &data->loc);
    }

    extern "C" UBSAN_FUNC void __ubsan_handle_shift_out_of_bounds(ubsan_shift_oob *data)
    {
        ubsan_print_location("shift out of bounds", &data->loc);
    }

    extern "C" UBSAN_FUNC void __ubsan_handle_load_invalid_value(ubsan_invalid_value *data)
    {
        ubsan_print_location("load invalid value value", &data->loc);
    }

    extern "C" UBSAN_FUNC void __ubsan_handle_out_of_bounds(ubsan_array_oob *data)
    {
        ubsan_print_location("out of bounds", &data->loc);
    }

    extern "C" UBSAN_FUNC void __ubsan_handle_type_mismatch_v1(ubsan_type_mismatch *data, uintptr_t ptr)
    {
        const char *type_check_types[10] =
        {
            "load of",
            "store to",
            "reference binding to",
            "member access within",
            "member call on",
            "constructor call on",
            "downcast of",
            "downcast of",
            "upcast of",
            "cast to virtual base of",
        };

        if (ptr == 0)
            ubsan_print_location("null pointer access", &data->loc);
        else if (data->align != 0 && !(ptr & (data->align - 1)))
            ubsan_print_location("unaligned access", &data->loc);
        else
        {
            if (data->check_kind >= 10)
                ubsan_print_location("type mismatch", &data->loc);
            else
            {
                if (ubsan_lock)
                    return;
                ubsan_lock = true;
                TTY::Print("%s address of %p doesn't have enough space for an object %s\n", type_check_types[data->check_kind], (uintptr_t)ptr, data->type->name);
                ubsan_lock = false;
                ubsan_print_location("type mismatch", &data->loc);
            }
        }
    }

    extern "C" UBSAN_FUNC void __ubsan_handle_vla_bound_not_positive(ubsan_negative_vla *data)
    {
        ubsan_print_location("negative vla index", &data->loc);
    }

    extern "C" UBSAN_FUNC void __ubsan_handle_nonnull_return(ubsan_non_null_return *data)
    {
        ubsan_print_location("non-null return has null", &data->loc);
    }

    extern "C" UBSAN_FUNC void __ubsan_handle_nonnull_arg(ubsan_non_null_argument *data)
    {
        ubsan_print_location("non-null argument has null", &data->loc);
    }

    extern "C" UBSAN_FUNC void __ubsan_handle_builtin_unreachable(struct ubsan_unreachable *data)
    {
        ubsan_print_location("unreachable code", &data->loc);
    }

    extern "C" UBSAN_FUNC void __ubsan_handle_invalid_builtin(struct ubsan_invalid_builtin *data)
    {
        ubsan_print_location("invalid builtin", &data->loc);
    }
}
