set(DasHLE_memory_allocate_SOURCES 
    ${DasHLE_SOURCES}
    ./Allocate.cpp
)
add_executable(DasHLE_memory_allocate ${DasHLE_memory_allocate_SOURCES})

set(DasHLE_memory_free_SOURCES 
    ${DasHLE_SOURCES}
    ./Free.cpp
)
add_executable(DasHLE_memory_free ${DasHLE_memory_free_SOURCES})

set(DasHLE_memory_OOM_SOURCES 
    ${DasHLE_SOURCES}
    ./OOM.cpp
)
add_executable(DasHLE_memory_OOM ${DasHLE_memory_OOM_SOURCES})