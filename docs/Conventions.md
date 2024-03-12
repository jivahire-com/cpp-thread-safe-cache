
## Application

An "app" or application is a firmware image compiled to run on a core. The application includes a threadx kernel, any number of drivers, any number of threads, and any number of services that run within threads.

## Service

@TODO - Update this section based on shared service design: https://azurecsi.visualstudio.com/DuvallFw/_workitems/edit/1473181

A service is a special type of library that exposes an interface that can be consumed within the same thread, or by other threads, or even code in the kernel including drivers.

A service has interfaces:

### Service Hosting Interface

The service hosting interface is a plain-C interface that the hosting environment (the thread) calls by linking in the service. The minimum requirement for this interface is that it provide an initialization function that returns a handle for the services's user interface.

To host a service from a thread, link the service target and then call the service's initialization routine:

```cmake
target_link_libraries(
    my_thread
    PUBLIC
        ms::m_foo_service
)
```

```C
#include "servicehost/foo_host.h"

void my_thread_init() {
    foo_t foo_handle = foo_service_initialize();
}
```

## Driver

@TODO - Add documentation to this section
        https://azurecsi.visualstudio.com/DuvallFw/_workitems/edit/1478129

## Target naming convention

Targets should be named with the convention `<namespace>_<component name>(_<component type/>)`
Example `ms_uart_driver`

Each target that may be linked from a separate .cmake file should define an alias with the `_` characters replaced with `::`
Example `ms::uart::driver`

If a target name does not include `::` characters and cmake cannot find the target, cmake will assume the name refers to a plain pre-compiled library file and not a cmake target. This can have unexpected results in the build. By exporting target aliases with a special namespace, `target_link_libraries` is unambiguous.

### namespace
The namespace should be `ms` short "Microsoft" for components developed within the project. For externally provided libraries use an appropriate identifier (e.g. `threadx`)

### component name
The name of the component. e.g. `uart`, `dvfs`, `crash_dump`, etc.

### component type
Many components will need to build multiple targets for the same logical component. Specifying the component type in the name
helps to easily identify the role of the target.

The "type" of the component if it is one of the standard patterns. If the component is a plain library this may be omitted.

* `service`
    * The implementation of a service library
    * A service library is hosted in a library
* `driver`
    * The implementation of a driver library
    * A driver library is linked in to a kernel
* `kernel`
    * The "kernel" flavor of a library that is compiled for either environment

Example:
```cmake
add_library(ms::io::service ALIAS ${TARGET})
```
