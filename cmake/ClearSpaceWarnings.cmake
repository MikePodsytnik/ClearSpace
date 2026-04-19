function(clearspace_enable_warnings target_name)
  if (MSVC)
    target_compile_options(${target_name} PRIVATE /W4 /permissive- /EHsc)
  else()
    target_compile_options(${target_name} PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wshadow
      -Wnon-virtual-dtor
      -Wold-style-cast
      -Woverloaded-virtual
      -Wnull-dereference
    )
  endif()
endfunction()

function(clearspace_enable_sanitizer target_name)
  if (CLEARSPACE_SANITIZER STREQUAL "none")
    return()
  endif()

  if (MSVC)
    message(WARNING "Sanitizers via flags are not configured for MSVC in this project.")
    return()
  endif()

  if (CLEARSPACE_SANITIZER STREQUAL "asan")
    target_compile_options(${target_name} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(${target_name} PRIVATE -fsanitize=address,undefined)
  elseif (CLEARSPACE_SANITIZER STREQUAL "tsan")
    target_compile_options(${target_name} PRIVATE -fsanitize=thread -fno-omit-frame-pointer)
    target_link_options(${target_name} PRIVATE -fsanitize=thread)
  elseif (CLEARSPACE_SANITIZER STREQUAL "ubsan")
    target_compile_options(${target_name} PRIVATE -fsanitize=undefined -fno-omit-frame-pointer)
    target_link_options(${target_name} PRIVATE -fsanitize=undefined)
  else()
    message(FATAL_ERROR "Unknown CLEARSPACE_SANITIZER: ${CLEARSPACE_SANITIZER}")
  endif()
endfunction()
