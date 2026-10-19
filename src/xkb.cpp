#include "xkb.hpp"

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <memory>

#include "config.hpp"
#include "seat.hpp"
#include "util.hpp"
#include "wm.hpp"

namespace {

river_xkb_keymap_v1 *createKeymap(river_xkb_config_v1 *xkbConfig) {
    xkb_rule_names names = {};
    names.layout = config.xkb.layout.c_str();
    names.variant = config.xkb.variant.c_str();

    xkb_keymap *keymap = xkb_keymap_new_from_names2(
        wm.globals.xkbContext, &names, XKB_KEYMAP_FORMAT_TEXT_V2,
        XKB_KEYMAP_COMPILE_NO_FLAGS);
    if (keymap == nullptr) {
        diag::error("failed to create xkb keymap");
        return nullptr;
    }

    std::unique_ptr<char, decltype(&std::free)> text(
        xkb_keymap_get_as_string2(keymap, XKB_KEYMAP_FORMAT_TEXT_V2,
                                  XKB_KEYMAP_SERIALIZE_NO_FLAGS),
        &std::free);
    xkb_keymap_unref(keymap);
    if (!text) {
        diag::error("failed to serialize xkb keymap");
        return nullptr;
    }

    const size_t size = std::strlen(text.get()) + 1;
    UniqueFd fd(memfd_create("taiga-keymap", MFD_CLOEXEC | MFD_ALLOW_SEALING));
    if (!fd.valid() || ftruncate(fd.get(), static_cast<off_t>(size)) < 0) {
        diag::error("failed to create or truncate mem fd");
        return nullptr;
    }

    void *mapped =
        mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd.get(), 0);
    if (mapped == MAP_FAILED) {
        diag::error("failed to map keymap data");
        return nullptr;
    }
    std::memcpy(mapped, text.get(), size);
    if (munmap(mapped, size) < 0) {
        diag::error("failed to unmap keymap data");
        return nullptr;
    }

    if (fcntl(fd.get(), F_ADD_SEALS,
              F_SEAL_SHRINK | F_SEAL_GROW | F_SEAL_WRITE | F_SEAL_SEAL) < 0) {
        diag::error("failed to seal mem fd");
        return nullptr;
    }

    return river_xkb_config_v1_create_keymap(xkbConfig, fd.get(),
                                             XKB_KEYMAP_FORMAT_TEXT_V2);
}

const river_xkb_keymap_v1_listener keymapListener = {
    .success = noop,
    .failure =
        [](void *, river_xkb_keymap_v1 *, const char *message) {
            diag::error("compositor rejected keymap: %s", message);
        },
};

} // namespace

const river_xkb_config_v1_listener xkbConfigListener = {
    .finished = noop,
    .xkb_keyboard =
        [](void *, river_xkb_config_v1 *xkbConfig,
           river_xkb_keyboard_v1 *keyboard) {
            river_xkb_keymap_v1 *keymap = createKeymap(xkbConfig);
            if (keymap == nullptr) {
                diag::error("failed to create keymap.");
                return;
            }
            river_xkb_keymap_v1_add_listener(keymap, &keymapListener, nullptr);
            river_xkb_keyboard_v1_set_keymap(keyboard, keymap);
        },
};

const river_xkb_binding_v1_listener xkbBindingListener = {
    .pressed =
        [](void *data, river_xkb_binding_v1 *) {
            auto *binding = static_cast<XkbBinding *>(data);
            binding->seat->pendingAction = binding->action;
            binding->seat->pendingArg = binding->arg;
        },
    .released = noop,
    .stop_repeat = noop,
};

void XkbBinding::destroy() { river_xkb_binding_v1_destroy(obj); }

std::unique_ptr<XkbBinding> createXkbBinding(Seat &seat, uint32_t mods,
                                             xkb_keysym_t keysym, Action action,
                                             const std::string &arg) {
    auto binding = std::make_unique<XkbBinding>();
    binding->obj = river_xkb_bindings_v1_get_xkb_binding(
        wm.globals.xkbBindings, seat.obj, keysym, mods);
    binding->seat = &seat;
    binding->action = action;
    binding->arg = arg;

    river_xkb_binding_v1_add_listener(binding->obj, &xkbBindingListener,
                                      binding.get());
    river_xkb_binding_v1_enable(binding->obj);
    return binding;
}
