#include "pointer.hpp"

#include "seat.hpp"
#include "util.hpp"

const river_pointer_binding_v1_listener pointerBindingListener = {
    .pressed =
        [](void *data, river_pointer_binding_v1 *) {
            auto *binding = static_cast<PointerBinding *>(data);
            binding->seat->pendingAction = binding->action;
            binding->seat->pendingArg.clear();
        },
    .released = noop,
};

void PointerBinding::destroy() { river_pointer_binding_v1_destroy(obj); }

std::unique_ptr<PointerBinding> createPointerBinding(Seat &seat, uint32_t mods,
                                                     uint32_t button,
                                                     Action action) {
    auto binding = std::make_unique<PointerBinding>();
    binding->obj = river_seat_v1_get_pointer_binding(seat.obj, button, mods);
    binding->seat = &seat;
    binding->action = action;

    river_pointer_binding_v1_add_listener(binding->obj, &pointerBindingListener,
                                          binding.get());
    river_pointer_binding_v1_enable(binding->obj);
    return binding;
}
