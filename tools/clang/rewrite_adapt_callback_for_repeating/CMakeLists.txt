set(LLVM_LINK_COMPONENTS
  BitReader
  MCParser
  Option
  Support
  X86AsmParser
  X86CodeGen
  X86Desc
  X86Info
  )

add_llvm_executable(rewrite_adapt_callback_for_repeating
  RewriteAdaptCallbackForRepeating.cpp
  )

target_link_libraries(rewrite_adapt_callback_for_repeating
  clangAST
  clangASTMatchers
  clangAnalysis
  clangBasic
  clangDriver
  clangEdit
  clangFrontend
  clangLex
  clangParse
  clangSema
  clangSerialization
  clangTooling
  clangToolingCore
  )

cr_install(TARGETS rewrite_adapt_callback_for_repeating RUNTIME DESTINATION bin)
