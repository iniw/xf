#include "blinky.hpp"
#include "messenger.hpp"

class Maestro {
public:
    void create() {
        event_queue.create();
        messenger.create(1);
        blinky.create(2);
    }

private:
    event::Queue event_queue;
    Blinky blinky { event_queue };
    Messenger messenger { event_queue };
};

static Maestro g_maestro;

extern "C" void app_main(void) {
    g_maestro.create();
}
