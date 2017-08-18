#ifndef DOOGIE_BLOCKER_RULES_H_
#define DOOGIE_BLOCKER_RULES_H_

#include <QtWidgets>
#include <memory>

namespace doogie {

class BlockerRules {
 public:
  class CommentRule;
  class StaticRule;
  class CosmeticRule;
  class Rule {
   public:
    static Rule ParseRule(const QString& line,
                          int file_index,
                          int line_num);

    virtual bool IsNull() const { return true; }
    virtual const CommentRule* AsComment() const { return nullptr; }
    virtual const StaticRule* AsStatic() const { return nullptr; }
    virtual const CosmeticRule* AsCosmetic() const { return nullptr; }

    int FileIndex() const { return file_index_; }
    int LineNum() const { return line_num_; }

   private:
    int file_index_ = -1;
    int line_num_ = -1;
  };

  class CommentRule : public Rule {
   public:
    static CommentRule ParseRule(const QString& line);

    CommentRule();
    bool IsNull() const override { return line_.isNull(); }
    const CommentRule* AsComment() const override { return this; }
   private:
    QString line_;
  };

  class StaticRule : public Rule {
   public:
    enum RuleMatch {
      None,
      Any,
      SingleSep,
      Lit,
      MustEnd,
      MustStart
    };

    struct MatchContext {
      const QByteArray& url;
    };

    class RulePiece {
     public:
      RulePiece();
      explicit RulePiece(const QByteArray& piece);
      explicit RulePiece(RuleMatch match);

      const RulePiece* Match(const MatchContext& ctx, int curr_index) const;
      void AddAsChildOf(RulePiece* parent);
      void SetAsTerminator(int file_index, int line_num) {
        file_index_ = file_index;
        line_num_ = line_num;
      }

      bool IsNull() const { return match_ == None; }

     private:
      QByteArray piece_;
      RuleMatch match_ = None;
      int file_index_ = -1;
      int line_num_ = -1;
      // Key is 0 for any non-lit rules. Note, can have
      //  multiple values per char
      QHash<char, RulePiece> children_;
    };

    static StaticRule ParseRule(const QString& line,
                                int file_index,
                                int line_num);

    StaticRule();
    bool IsNull() const override { return root_piece_.IsNull(); }
    const StaticRule* AsStatic() const override { return this; }

   private:
    bool starts_with_domain_ = false;
    RulePiece root_piece_;
  };

  class CosmeticRule : public Rule {
   public:
    static CosmeticRule ParseRule(const QString& line);

    bool IsNull() const override {
      // TODO(cretz): this
      return true;
    }
    const CosmeticRule* AsCosmetic() const override { return this; }
  };

  static QList<Rule> ParseRules(QTextStream* stream, int file_index);

 private:
  QHash<QString, StaticRule::RulePiece> domain_specific_;
  StaticRule::RulePiece root_general_rule_;
};

}  // namespace doogie

#endif  // DOOGIE_BLOCKER_RULES_H_
