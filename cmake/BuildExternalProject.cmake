# From http://stackoverflow.com/a/23570741/6878912
# This function is used to force a build on a dependant project at cmake configuration phase.

function (build_external_project target url cmake_args)

    set(trigger_build_dir ${CMAKE_BINARY_DIR}/force_${target})

    #mktemp dir in build tree
    file(MAKE_DIRECTORY ${trigger_build_dir} ${trigger_build_dir}/build)

    #generate false dependency project
    set(CMAKE_LIST_CONTENT "
    cmake_minimum_required(VERSION 2.8)

    include(ExternalProject)
    ExternalProject_add(${target}
            URL ${url}
            PREFIX ${CMAKE_BINARY_DIR}/ext/${target}
            CMAKE_ARGS ${cmake_args} #-DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_BINARY_DIR}/ext/${target}
            )

            add_custom_target(trigger_${target})
            add_dependencies(trigger_${target} ${target})")

    file(WRITE ${trigger_build_dir}/CMakeLists.txt "${CMAKE_LIST_CONTENT}")

    execute_process(COMMAND ${CMAKE_COMMAND} ..
        WORKING_DIRECTORY ${trigger_build_dir}/build
        )
    execute_process(COMMAND ${CMAKE_COMMAND} --build .
        WORKING_DIRECTORY ${trigger_build_dir}/build
        )

endfunction()
