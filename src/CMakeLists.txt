add_subdirectory(common)
add_subdirectory(login)
add_subdirectory(map)
add_subdirectory(profile)
add_subdirectory(search)
add_subdirectory(world)
add_subdirectory(test)

# Not used by this server, so left out of the default build. Still buildable with --target <name>.
set_target_properties(xi_connect xi_profile xi_profile_lib PROPERTIES EXCLUDE_FROM_ALL TRUE)
