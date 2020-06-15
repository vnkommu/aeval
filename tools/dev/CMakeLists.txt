add_executable (dev Dev.cpp)
target_link_libraries (dev ${Z3_LIBRARY} ${Boost_SYSTEM_LIBRARY} ${GMPXX_LIB} ${GMP_LIB})
llvm_config (dev bitwriter)
install(TARGETS dev RUNTIME DESTINATION bin)
