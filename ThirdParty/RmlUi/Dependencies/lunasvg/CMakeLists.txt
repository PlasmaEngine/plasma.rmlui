pl_cmake_init()

pl_requires_rmlui()

set (PROJECT_NAME "lunasvg")

pl_create_target(STATIC_LIBRARY ${PROJECT_NAME} NO_UNITY NO_PL_PREFIX)

# override the default IDE folder
pl_set_project_ide_folder(${PROJECT_NAME} "${PL_ROOT}/Code/ThirdParty/lunasvg")

target_compile_definitions(${PROJECT_NAME}
  PRIVATE
  LUNASVG_BUILD
  PLUTOVG_BUILD_STATIC
  _CRT_SECURE_NO_WARNINGS

  PUBLIC
  LUNASVG_BUILD_STATIC
  BUILDSYSTEM_ENABLE_LUNASVG_SUPPORT
)

target_include_directories(${PROJECT_NAME}
  PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}/plutovg/include"

  PUBLIC
  "${CMAKE_CURRENT_LIST_DIR}/include"
)

if(PL_CMAKE_COMPILER_MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /wd4018 /wd4244 /wd4267 /wd4456 /wd4458 /wd4701)
endif()
