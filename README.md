# Goals
As the name (`xf` - e<b>X</b>tension to <b>F</b>reertos) might suggest, the goal of this library is to extend FreeRTOS - to make it more ergonomic, convenient and safer all while honouring it's original design choices. This means that the overall structure, naming and usage patterns of `xf` should be highly familiar to any developer used to FreeRTOS.

Here are some of the things that this library aims to achieve, in no particular order:
- ### Faster development speed
    FreeRTOS is written in mildly object oriented C, meaning it's naturally heavy on boilerplate, macros, opaque pointers, etc. which create friction between the developer and their goals. It's easy to find yourself spending a considerable amount of time performing FreeRTOS' ceremonies instead of writing the actual features. By utilizing modern C++ most of that can go away, resulting in a thiner, less intrusive and more intuitive API surface that makes things *easier*, instead of harder.

- ### Easy to reason about, with little to no magic
    While this library does use and require a modern C++ compiler, effort was made to avoid pilling on modern features for the sake of it. Function signatures and templates should be minimal and readable, familiar even to those not used to the newer toys like concepts.

- ### A consistently improved experience
    When met with the choice between a raw FreeRTOS "class" and it's `xf` alternative, it should be a no-brainer answer. `xf`'s version of FreeRTOS functionalities should be strictly better in all reasonable categories: safety, ease of use, expressiveness, etc.

    The "consistently" part of this point is that `xf`'s alternatives should never feel alien, over the top or a reinvention of the wheel. The steps done to improve feature A should also show up in feature B, and they should make sense.

- ### Good documentation
    Documentation is provided in the code itself and is consistent and easy to cross-reference with FreeRTOS's. Here's an example:
    ```cpp
    /// Waits up to `timeout` amount of time for the item to be pushed to the back of the queue and returns whether it successfully did so.
    /// Refer to the module-level `time` documentation to see how time is converted to FreeRTOS ticks.
    /// Analogous to [`xQueueSend`](https://www.freertos.org/Documentation/02-Kernel/04-API-references/06-Queues/03-xQueueSend).
    template<typename Rep, typename Period>
    [[nodiscard]] bool send(const Item&, std::chrono::duration<Rep, Period> timeout);
    ```

- ### Tight integration with C++'s standard library
    `xf` utilizes `std::chrono`, `std::optional`, `<type_traits>` and other `std` niceties to provide a consistent and familiar API to C++ programmers.

# Usage
Check out the [examples](./examples) folder to get a feel for what `xf` looks like.

# Installation
As of this moment, `xf` has only been tested with the ESP-IDF build system and the ESP32 family of microcontrollers, but care has been taken to never use or expose Espressif-specific APIs. It should be pretty painless to port it to other build systems and chips.

When using ESP-IDF, place this repository in your project's `components/` directory and then add it as a requirement of your `main` component:

```cmake
# main/CMakeLists.txt
idf_component_register(
    SRCS
        main.cpp
        # ...
    PRIV_REQUIRES
        xf
        # ...
)
```

The easiest way to do that is to just clone the repository directly to that folder:
```sh
# (from your project's root directory)
$ git clone https://github.com/iniw/xf.git components/
```

You could also add it as a git submodule, which makes it easier to update:
```sh
# (from your project's root directory)
$ git submodule add https://github.com/iniw/xf.git components/
```

You can also consume it through CMake's FetchContent module, which allows pinning to a specific release tag, see [this example](./examples/cmake-fetch-content/CMakeLists.txt).
