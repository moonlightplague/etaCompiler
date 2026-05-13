add_library(etac_frontend
    lexer_driver.cpp
    parser_driver.cpp
    typechecker_driver.cpp
    ir_driver.cpp
    ast/ast.cpp
    ir/ir.cpp
    generated/etaLex.cpp
    generated/etaParse.cpp
    generated/etaParseBaseListener.cpp
    generated/etaParseListener.cpp
    generated/etaParseBaseVisitor.cpp
    generated/etaParseVisitor.cpp
    utils.cpp
        ast/EtaTypeChecker.cpp
    ir/EtaMIRtoLIRVisitor.cpp
    ir/EtaASTtoIRVisitor.cpp
    ir/EtaConstantFolder.cpp
    ir/EtaIRControlFlowAnalyzer.cpp
    cfg/cfg.cpp
    cfg/CfgDotWriter.cpp
    cfg/EtaIRtoCFGVisitor.cpp
)

target_include_directories(etac_frontend PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/generated
    ${PROJECT_SOURCE_DIR}/include/ast
    ${PROJECT_SOURCE_DIR}/src/ast
    ${PROJECT_SOURCE_DIR}/include/ir
    ${PROJECT_SOURCE_DIR}/src/ir
    ${PROJECT_SOURCE_DIR}/libs
    ${PROJECT_SOURCE_DIR}/libs/antlr4/runtime/Cpp/runtime/src
    ${PROJECT_SOURCE_DIR}/include/cfg
)

target_link_libraries(etac_frontend PUBLIC
    antlr4_static
)

add_library(
        etac_backend
    asm_driver.cpp
    cfg/AsmCfg.cpp
    codegen/asm.cpp
    codegen/EtaIRtoASMVisitor.cpp
    codegen/EtaTrivialRegisterAllocationVisitor.cpp
)

target_include_directories(etac_backend PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/include/codegen
    ${PROJECT_SOURCE_DIR}/include/cfg
    ${PROJECT_SOURCE_DIR}/src/codegen
    ${PROJECT_SOURCE_DIR}/include/ir
    ${PROJECT_SOURCE_DIR}/src/ir
    ${PROJECT_SOURCE_DIR}/src/cfg
)
