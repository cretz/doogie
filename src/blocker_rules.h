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
    static Rule* ParseRule(const QString& line,
                           int file_index,
                           int line_num);

    virtual CommentRule* AsComment() { return nullptr; }
    virtual StaticRule* AsStatic() { return nullptr; }
    virtual CosmeticRule* AsCosmetic() { return nullptr; }

    int FileIndex() const { return file_index_; }
    int LineNum() const { return line_num_; }

   private:
    int file_index_ = -1;
    int line_num_ = -1;
  };

  class CommentRule : public Rule {
   public:
    static CommentRule* ParseRule(const QString& line);

    CommentRule* AsComment() override { return this; }
   private:
    QString line_;
  };

  class StaticRule : public Rule {
   public:
    enum RequestType {
      AllRequests,  // Only used for the hash, never inside the rule
      Script,
      Image,
      Stylesheet,
      Object,
      XmlHttpRequest,
      ObjectSubRequest,
      SubDocument,
      Ping,
      WebSocket,
      WebRtc,
      Document,
      ElemHide,
      GenericHide,
      GenericBlock,
      Popup,
      Other
    };

    enum RequestParty {
      AnyParty,
      ThirdParty,
      FirstParty
    };

    enum CollapseOption {
      DefaultCollapse,
      AlwaysHide,
      NeverHide
    };

    struct MatchContext {
      RequestType request_type;
      RequestParty request_party;
      QByteArray target_url;
      // These are each sub host piece. So if the hostname is foo.bar.baz.com
      //  then this will have foo.bar.baz.com, bar.baz.com, and baz.bom in it.
      QSet<QByteArray> target_hosts;
      // The character index of the first character after the hostname.
      int target_url_after_host_index;
      QByteArray ref_url;
      // Same sub piece array as in target_hosts.
      QSet<QByteArray> ref_hosts;
    };

    class RulePiece {
     public:
      RulePiece();
      explicit RulePiece(const QByteArray& piece);

      bool IsNull() const { return piece_.isNull(); }
      StaticRule* RuleThisTerminates() const { return rule_this_terminates_; }
      void MakeRootIfNull() {
        if (piece_.isNull()) piece_ = "*";
      }
      void AppendRule(StaticRule* rule, int piece_index = 0);

      const RulePiece* CheckMatch(const MatchContext& ctx,
                                  int curr_index = 0) const;

      void RuleTree(QJsonObject* obj) const;

      static const RulePiece& kNull;

     private:
      QByteArray piece_;
      bool case_sensitive_ = false;
      // Key is 0 for any non-lit rules. Note, can have
      //  multiple values per char
      QHash<char, RulePiece> children_;

      // This is checked at the end for things like not-domain
      StaticRule* rule_this_terminates_ = nullptr;
    };

    static StaticRule* ParseRule(const QString& line,
                                 int file_index,
                                 int line_num);

    StaticRule* AsStatic() override { return this; }

    const QSet<RequestType>& RequestTypes() const { return request_types_; }
    const QSet<RequestType>& NotRequestTypes() const {
      return not_request_types_;
    }
    const QSet<QByteArray>& RefDomains() const { return ref_domains_; }
    const QSet<QByteArray>& NotRefDomains() const { return not_ref_domains_; }
    bool Exception() const { return exception_; }
    bool CaseSensitive() const { return case_sensitive_; }
    RequestParty ReqParty() const { return request_party_; }
    CollapseOption Collapse() const { return collapse_option_; }
    const QByteArray& TargetDomainName() const { return target_domain_name_; }
    const QVector<QByteArray>& Pieces() const { return pieces_; }

    static const QHash<QByteArray, RequestType> kRequestTypeStrings;

   private:
    QSet<RequestType> request_types_;
    // This is checked in the rule itself
    QSet<RequestType> not_request_types_;
    QSet<QByteArray> ref_domains_;
    // This is checked in the rule itself
    QSet<QByteArray> not_ref_domains_;
    bool exception_ = false;
    bool case_sensitive_ = false;
    RequestParty request_party_ = AnyParty;
    CollapseOption collapse_option_ = DefaultCollapse;
    QByteArray target_domain_name_;
    QVector<QByteArray> pieces_;
  };

  class CosmeticRule : public Rule {
   public:
    static CosmeticRule* ParseRule(const QString& line);

    CosmeticRule* AsCosmetic() override { return this; }
  };

  static QList<Rule*> ParseRules(QTextStream* stream,
                                 int file_index,
                                 bool* parse_ok = nullptr);

  ~BlockerRules();

  QJsonObject RuleTree() const;

  bool AddRules(QTextStream* stream, int file_index);
  // This takes ownership of all rules and will delete them on destruct
  void AddRules(const QList<Rule*>& rules);

  StaticRule* FindStaticRule(const QString& target_url,
                             const QString& ref_url,
                             StaticRule::RequestType request_type) const;

 private:
  // Keyed by the request type or AllRequests if it applies to all.
  typedef QHash<StaticRule::RequestType, StaticRule::RulePiece> RuleHash;
  // Keyed by the target domain. The rules within expect to have the scheme
  //  and the domain removed. If the rule is not domain specific, it's in
  //  the empty string form and the domain does not need to be removed.
  typedef QHash<QByteArray, RuleHash> TargetHostHash;
  // Keyed by the ref domain. Non-ref-domain-specific rules are in the empty
  //  string key.
  typedef QHash<QByteArray, TargetHostHash> RefHostHash;
  // Keyed by whether the rule is for first party, third party, or both
  typedef QHash<StaticRule::RequestParty, RefHostHash> PartyOptionHash;

  StaticRule* FindStaticRule(const StaticRule::MatchContext& ctx) const;
  StaticRule* FindStaticRuleInRefHostHash(
      const StaticRule::MatchContext& ctx, const RefHostHash& hash) const;
  StaticRule* FindStaticRuleInTargetHostHash(
      const StaticRule::MatchContext& ctx, const TargetHostHash& hash) const;
  StaticRule* FindStaticRuleInRuleHash(
      const StaticRule::MatchContext& ctx, const RuleHash& hash) const;

  void AddStaticRule(StaticRule* rule);
  void AddStaticRuleToRefHostHash(StaticRule* rule, RefHostHash* hash);
  void AddStaticRuleToTargetHostHash(StaticRule* rule, TargetHostHash* hash);
  void AddStaticRuleToRuleHash(StaticRule* rule, RuleHash* hash);

  QJsonObject RuleTreeForPartyHash(const PartyOptionHash& hash) const;
  QJsonObject RuleTreeForRefHostHash(const RefHostHash& hash) const;
  QJsonObject RuleTreeForTargetHostHash(const TargetHostHash& hash) const;
  QJsonObject RuleTreeForRuleHash(const RuleHash& hash) const;

  PartyOptionHash static_rules_;
  PartyOptionHash static_rule_exceptions_;

  // We only keep this so we can delete em all on destruct
  QList<Rule*> known_rules_;
};

}  // namespace doogie

#endif  // DOOGIE_BLOCKER_RULES_H_
