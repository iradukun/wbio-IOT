#include "WioTMobility.h"
#define GET_ARG_INSTANCIATION
#include "DeviceFunction.h"
#include <stdexcept>

template<> int GetArg<int>(duk_context* ctx, int nArg)
{
    if (duk_get_top(ctx) < nArg)
    {
        LogErr("WioT", "[CMDPROMPT] Not enough argument.");
        throw std::runtime_error("Not enough argument");
    }
    if (!duk_is_number(ctx, nArg))
    {
        LogErr("WioT", "[CMDPROMPT] Argument is not an integer.");
        throw std::runtime_error("Invalid type (waiting for an integer)");
    }
    duk_double_t value = duk_get_number(ctx, nArg);
    duk_remove(ctx, nArg);
    return static_cast<int>(value);
}

template<> unsigned GetArg<unsigned>(duk_context* ctx, int nArg)
{
    if (duk_get_top(ctx) < nArg)
    {
        LogErr("WioT", "[CMDPROMPT] Not enough argument.");
        throw std::runtime_error("Not enough argument");
    }
    if (!duk_is_number(ctx, nArg))
    {
        LogErr("WioT", "[CMDPROMPT] Argument is not an integer.");
        throw std::runtime_error("Invalid type (waiting for an integer)");
    }
    duk_double_t value = duk_get_number(ctx, nArg);
    duk_remove(ctx, nArg);
    return static_cast<unsigned>(value);
}

template<> uint64_t GetArg<uint64_t>(duk_context* ctx, int nArg)
{
    if (duk_get_top(ctx) < nArg)
    {
        LogErr("WioT", "[CMDPROMPT] Not enough argument.");
        throw std::runtime_error("Not enough argument");
    }
    if (!duk_is_number(ctx, nArg))
    {
        LogErr("WioT", "[CMDPROMPT] Argument is not an integer.");
        throw std::runtime_error("Invalid type (waiting for an integer)");
    }
    duk_double_t value = duk_get_number(ctx, nArg);
    duk_remove(ctx, nArg);
    return static_cast<uint64_t>(value);
}

template<> bool GetArg<bool>(duk_context* ctx, int nArg)
{
    if (duk_get_top(ctx) < nArg)
    {
        LogErr("WioT", "[CMDPROMPT] Not enough argument.");
        throw std::runtime_error("Not enough argument");
    }
    if (!duk_is_boolean(ctx, nArg))
    {
        LogErr("WioT", "[CMDPROMPT] Argument is not a boolean.");
        throw std::runtime_error("Invalid type (waiting for a boolean)");
    }
    duk_bool_t value = duk_get_boolean(ctx, nArg);
    duk_remove(ctx, nArg);
    return value;
}

template<> std::string GetArg<std::string>(duk_context* ctx, int nArg)
{
    if (duk_get_top(ctx) < nArg)
    {
        LogErr("WioT", "[CMDPROMPT] Not enough argument.");
        throw std::runtime_error("Not enough argument");
    }
    if (!duk_is_string(ctx, nArg))
    {
        LogErr("WioT", "[CMDPROMPT] Argument is not a string.");
        throw std::runtime_error("Invalid type (waiting for a string)");
    }
    std::string value = duk_get_string(ctx, nArg);
    duk_remove(ctx, nArg);
    return value;
}

template<> float GetArg<float>(duk_context* ctx, int nArg)
{
    if (duk_get_top(ctx) < nArg)
    {
        LogErr("WioT", "[CMDPROMPT] Not enough argument.");
        throw std::runtime_error("Not enough argument");
    }
    if (!duk_is_number(ctx, nArg))
    {
        LogErr("WioT", "[CMDPROMPT] Argument is not a float.");
        throw std::runtime_error("Invalid type (waiting for a float)");
    }
    duk_double_t value = duk_get_number(ctx, nArg);
    duk_remove(ctx, nArg);
    return static_cast<float>(value);
}

