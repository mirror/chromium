#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecordLayout.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/Sema/Sema.h"
#include "llvm/ADT/Triple.h"
#include "llvm/Support/raw_ostream.h"

#define DEVELOPMENT_MODE 0

class Visitor: public clang::RecursiveASTVisitor<Visitor> {
public:
  Visitor(clang::ASTContext& context): context_(context) {}

  bool shouldVisitTemplateInstantiations() const { return true; }

  bool VisitRecordDecl(clang::RecordDecl* record) {
    clang::QualType recordType = context_.getTypeDeclType(record);

#if DEVELOPMENT_MODE
    llvm::errs() << "LOOKING AT " << recordType.getAsString() << ": ";
#endif

    auto looksLike = [&](const char* reason) {
#if DEVELOPMENT_MODE
      llvm::errs() << reason << '\n';
#endif
    };

    if (record->isInvalidDecl()) {
      looksLike("invalid");
      return true;
    }

    if (!record->getDefinition()) {
      looksLike("no definition");
      return true;
    }

    if (record->isDependentContext()) {
      looksLike("dependent context");
      return true;
    }

    const clang::ASTRecordLayout& layout = context_.getASTRecordLayout(record);
    BitSize size = context_.toBits(layout.getSize());

    bool suspiciousRecord = isSuspiciousRecord(size);

    auto printRecordInfo = [&]() {
      clang::SourceLocation sourceLocation =
          context_.getSourceManager().getExpansionLoc(record->getLocStart());
      const llvm::Triple& triple = context_.getTargetInfo().getTriple();
      llvm::errs()
          << triple.getArchName() << "|"
          << sourceLocation.printToString(context_.getSourceManager()) << "|"
          << recordType.getAsString() << "|"
          << toBytes(size) << "|";
    };

#if DEVELOPMENT_MODE
    looksLike(suspiciousRecord ? "SUSPICIOUS" : "NOT suspicious");
    printRecordInfo();
    bool foundSuspiciousField = true;
#else
    if (!suspiciousRecord) {
      return true;
    }
    bool foundSuspiciousField = false;
#endif

    FieldPath fieldPath;
    visitFields(record, record, 0, fieldPath, [&](const FieldPath& path) {
#if !DEVELOPMENT_MODE
      if (foundSuspiciousField || !isSuspiciousField(path.back())) {
        return;
      }
      printRecordInfo();
      foundSuspiciousField = true;
#endif

#if DEVELOPMENT_MODE
      llvm::errs() << '\n';
      llvm::errs() << "  ";
#endif
      bool firstField = true;
      for (const auto& component: fieldPath) {
        clang::QualType recordType = context_.getTypeDeclType(component.record);
        clang::QualType fieldType = component.field->getType();
        BitSize fieldOffset = component.totalFieldOffset;
        BitSize fieldSize = context_.getTypeSize(fieldType);
        if (firstField) {
          firstField = false;
        } else {
          llvm::errs() << '#';
        }
        llvm::errs()
            << recordType.getAsString() << '$'
            << fieldType.getCanonicalType().getAsString() << '$'
            << component.field->getNameAsString() << ':'
            << toBytes(fieldOffset) << '-' << toBytes(fieldOffset + fieldSize);
      }
    });

    if (foundSuspiciousField) {
      llvm::errs() << '\n';
    }

    return true;
  }

private:
  using BitSize = int64_t;

  size_t toBytes(clang::CharUnits units) const {
    return toBytes(context_.toBits(units));
  }
  size_t toBytes(BitSize bits) const {
    return bits / 8;
  }

  struct FieldPathComponent {
    const clang::RecordDecl* record;
    const clang::FieldDecl* field;
    BitSize totalFieldOffset;
  };
  using FieldPath = std::vector<FieldPathComponent>;

  void visitFields(
      const clang::RecordDecl* rootRecord,
      const clang::RecordDecl* record,
      BitSize totalOffset,
      FieldPath& fieldPath,
      const std::function<void(const FieldPath&)>& visitor) const {

    const clang::ASTRecordLayout& layout = context_.getASTRecordLayout(record);

    for (const clang::FieldDecl* field: record->fields()) {
      if (field->isUnnamedBitfield()) {
        continue;
      }
      clang::QualType fieldType = field->getType();
      size_t fieldIndex = field->getFieldIndex();
      BitSize fieldOffset = totalOffset + layout.getFieldOffset(fieldIndex);

      fieldPath.push_back({record, field, fieldOffset});

      const auto* fieldRecord = llvm::dyn_cast_or_null<clang::RecordDecl>(
          fieldType->getAsTagDecl());
      if (fieldRecord) {
        visitFields(rootRecord, fieldRecord, fieldOffset, fieldPath, visitor);
      } else {
        visitor(fieldPath);
      }

      fieldPath.pop_back();
    }

    if (auto* cxxRecord = llvm::dyn_cast<clang::CXXRecordDecl>(record)) {
      for (const clang::CXXBaseSpecifier& base: cxxRecord->bases()) {
        if (base.isVirtual()) {
          continue;
        }
        const clang::CXXRecordDecl *baseRecord =
            base.getType()->getAsCXXRecordDecl();
        visitFields(
            rootRecord,
            baseRecord,
            totalOffset +
                context_.toBits(layout.getBaseClassOffset(baseRecord)),
            fieldPath,
            visitor);
      }

      if (record == rootRecord) {
        for (const clang::CXXBaseSpecifier& base: cxxRecord->vbases()) {
          const clang::CXXRecordDecl *baseRecord =
              base.getType()->getAsCXXRecordDecl();
          visitFields(
              rootRecord,
              baseRecord,
              totalOffset +
                  context_.toBits(layout.getVBaseClassOffset(baseRecord)),
              fieldPath,
              visitor);
        }
      }
    }
  }

  bool isSuspiciousRecord(BitSize size) const {
    size_t suspiciousSizeFrom;
    size_t suspiciousSizeUpTo;
    const llvm::Triple& triple = context_.getTargetInfo().getTriple();
    if (triple.isArch32Bit()) {
      suspiciousSizeFrom = 25;
      suspiciousSizeUpTo = 32;
    } else {
      // TODO
      return false;
    }
    size_t sizeInBytes = toBytes(size);
    return sizeInBytes >= suspiciousSizeFrom &&
           sizeInBytes <= suspiciousSizeUpTo;
  }

#if !DEVELOPMENT_MODE
  bool isSuspiciousField(const FieldPathComponent& fieldInfo) const {
    const llvm::Triple& triple = context_.getTargetInfo().getTriple();
    size_t suspiciousOffsetFrom;
    if (triple.isArch32Bit()) {
      suspiciousOffsetFrom = 20;
    } else if (triple.isArch64Bit()) {
      suspiciousOffsetFrom = 40;
    } else {
      return false;
    }
    size_t suspiciousOffsetTo = suspiciousOffsetFrom +
        toBytes(context_.getTypeSize(context_.VoidPtrTy));

    BitSize fieldOffset = fieldInfo.totalFieldOffset;
    BitSize fieldSize = context_.getTypeSize(fieldInfo.field->getType());
    return toBytes(fieldOffset) >= suspiciousOffsetFrom &&
           toBytes(fieldOffset + fieldSize) <= suspiciousOffsetTo;
  }
#endif  // !DEVELOPMENT_MODE

  clang::ASTContext& context_;
};

class Consumer: public clang::ASTConsumer {
public:
  void HandleTranslationUnit(clang::ASTContext& context) override {
    Visitor(context).TraverseDecl(context.getTranslationUnitDecl());
  }
};

class Plugin: public clang::PluginASTAction {
protected:
  std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
      clang::CompilerInstance&,
      llvm::StringRef) override {
    return llvm::make_unique<Consumer>();
  }

  bool ParseArgs(const clang::CompilerInstance&,
                 const std::vector<std::string>&) override {
    return true;
  }
};

static clang::FrontendPluginRegistry::Add<Plugin> X(
    "find-suspicious-classes",
    "Finds suspicious classes (crbug.com/736675)");
