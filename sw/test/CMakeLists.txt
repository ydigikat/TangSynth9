set(UNITY_SRC ${CMAKE_CURRENT_SOURCE_DIR}/unity/unity.c)
set(FW_SRC    ${PROJECT_SOURCE_DIR}/source)

set(SYNTH_COMMON
    ${FW_SRC}/synth/env.c
    ${FW_SRC}/synth/lfo.c
    ${FW_SRC}/synth/luts.c
    ${FW_SRC}/synth/params.c
)

foreach(TEST_SRC test_midi.c test_voice.c test_synth.c test_lfo.c test_env.c)
    get_filename_component(TEST_NAME ${TEST_SRC} NAME_WE)

    if(TEST_SRC STREQUAL "test_midi.c")
        set(TEST_DEPS ${FW_SRC}/synth/midi.c)
    elseif(TEST_SRC STREQUAL "test_voice.c")
        set(TEST_DEPS
            ${FW_SRC}/synth/voice.c
            ${SYNTH_COMMON}
        )
    elseif(TEST_SRC STREQUAL "test_synth.c")
        set(TEST_DEPS
            ${FW_SRC}/synth/synth.c
            ${FW_SRC}/synth/voice.c
            ${FW_SRC}/synth/midi.c
            ${SYNTH_COMMON}
        )
    elseif(TEST_SRC STREQUAL "test_lfo.c")
        set(TEST_DEPS
            ${FW_SRC}/synth/lfo.c
            ${FW_SRC}/synth/luts.c
        )
    elseif(TEST_SRC STREQUAL "test_env.c")
        set(TEST_DEPS
            ${FW_SRC}/synth/env.c
            ${FW_SRC}/synth/luts.c
        )
    endif()

    add_executable(${TEST_NAME}
        ${TEST_SRC}
        ${UNITY_SRC}
        ${TEST_DEPS}
    )
    target_include_directories(${TEST_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/unity
        ${FW_SRC}
        ${FW_SRC}/bsp
        ${FW_SRC}/synth
    )
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endforeach()

enable_testing()