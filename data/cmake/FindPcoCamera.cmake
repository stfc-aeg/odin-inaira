message("\nLooking for PCO camera headers and libraries")

if (PCOCAMERA_ROOT_DIR)
    message(STATUS "Searching PCO camera root dir: ${PCOCAMERA_ROOT_DIR}")
endif()

if(PCOCAMERA_ROOT_DIR)

    set(PCOCAMERA_COMMON_DIR ${PCOCAMERA_ROOT_DIR}/pco_common)
    set(PCOCAMERA_CLHS_DIR ${PCOCAMERA_ROOT_DIR}/pco_clhs)
    set(PCOCAMERA_LIBRARY_DIR ${PCOCAMERA_COMMON_DIR}/pco_lib)
    set(PCOCAMERA_LIBRARIES pcoclhs pcocam_clhs pcolog pcofile)

    find_path(
        PCOCAMERA_COMMON_INCLUDE_DIR defs.h
        PATHS ${PCOCAMERA_COMMON_DIR}/pco_include
        NO_DEFAULT_PATH
    )

    find_path(
        PCOCAMERA_CLASSES_INCLUDE_DIR Cpco_com.h
        PATHS ${PCOCAMERA_COMMON_DIR}/pco_classes
        NO_DEFAULT_PATH
    )

    find_path(
        PCOCAMERA_CLHS_COMMON_INCLUDE_DIR pco_clhs_cam.h
        PATHS ${PCOCAMERA_CLHS_DIR}/pco_clhs_common
        NO_DEFAULT_PATH
    )

    find_path(
        PCOCAMERA_CLHS_CLASSES_INCLUDE_DIR Cpco_grab_clhs.h
        PATHS ${PCOCAMERA_CLHS_DIR}/pco_classes
        NO_DEFAULT_PATH
    )

    set(PCOCAMERA_LIBRARIES_DIR)
    foreach(LIB_NAME ${PCOCAMERA_LIBRARIES})
        set(LIB_VAR "LIB_${LIB_NAME}")
        find_library(${LIB_VAR} ${LIB_NAME} PATHS ${PCOCAMERA_LIBRARY_DIR})
        LIST(APPEND PCOCAMERA_LIBRARIES_DIR ${${LIB_VAR}})
    endforeach()

endif()

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(PCOCAMERA
    DEFAULT_MSG
    PCOCAMERA_COMMON_INCLUDE_DIR
    PCOCAMERA_CLASSES_INCLUDE_DIR
    PCOCAMERA_CLHS_COMMON_INCLUDE_DIR
    PCOCAMERA_CLHS_CLASSES_INCLUDE_DIR
    PCOCAMERA_LIBRARIES_DIR
)

if (PCOCAMERA_FOUND)

    set(PCOCAMERA_INCLUDES
        ${PCOCAMERA_COMMON_INCLUDE_DIR}
        ${PCOCAMERA_CLASSES_INCLUDE_DIR}
        ${PCOCAMERA_CLHS_COMMON_INCLUDE_DIR}
        ${PCOCAMERA_CLHS_CLASSES_INCLUDE_DIR}
    )

    message(STATUS "Include dirs: ${PCOCAMERA_INCLUDES}")
    message(STATUS "Library dir: ${PCOCAMERA_LIBRARY_DIR}")
    message(STATUS "Libraries: ${PCOCAMERA_LIBRARIES_DIR}")

else()
    message(STATUS "PCO camera package not found, not building PCO plugins")
endif()

