set(UNITY_SRC ${CMAKE_CURRENT_SOURCE_DIR}/unity/unity.c)
set(FW_SRC    ${PROJECT_SOURCE_DIR}/source)

foreach(TEST_SRC test_midi.c)
    get_filename_component(TEST_NAME ${TEST_SRC} NAME_WE)
    add_executable(${TEST_NAME}
        ${TEST_SRC}
        ${UNITY_SRC}
        ${FW_SRC}/synth/midi.c   
    )
    target_include_directories(${TEST_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/unity
        ${FW_SRC}
        ${FW_SRC}/synth
    )
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endforeach()

enable_testing()