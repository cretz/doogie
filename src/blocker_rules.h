#ifndef DOOGIE_BLOCKER_RULES_H_
#define DOOGIE_BLOCKER_RULES_H_

#include <QtWidgets>
#include <memory>
#include <bitset>

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

    QString MetadataKey() const;
    QString MetadataValue() const;

   private:
    QString line_;
  };

  class StaticRule : public Rule {
   public:
    enum RequestType {
      // Only used for the hash, never inside the rule
      AllRequests,
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
      Font,
      Media,
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
      MatchContext WithTargetUrlChanged(
          const QByteArray& new_target_url) const {
        return { request_type, request_party, new_target_url, target_hosts,
                 target_url_after_host_index, ref_url, ref_hosts };
      }

      const RequestType request_type;
      const RequestParty request_party;
      const QByteArray target_url;
      // These are each sub host piece. So if the hostname is foo.bar.baz.com
      //  then this will have foo.bar.baz.com, bar.baz.com, and baz.bom in it.
      const QSet<QByteArray> target_hosts;
      // The character index of the first character after the hostname.
      const int target_url_after_host_index;
      const QByteArray ref_url;
      // Same sub piece array as in target_hosts.
      const QSet<QByteArray> ref_hosts;
    };

    struct Info {
      // We choose vectors here because of size constraints
      std::bitset<Other> not_request_types;
      QSet<QByteArray> not_ref_domains;
      int file_index = -1;
      int line_num = -1;
    };

    class RulePiece {
     public:
      RulePiece();
      explicit RulePiece(const QByteArray& piece);
      ~RulePiece();

      bool IsNull() const { return children_.isEmpty() && !rule_this_terminates_; }
      Info* RuleThisTerminates() const {
        return rule_this_terminates_;
      }
      void AppendRule(StaticRule* rule, int piece_index = 0);

      const RulePiece* CheckMatch(const MatchContext& ctx,
                                  int curr_index = 0) const;

      void RuleTree(QJsonObject* obj) const;

      static const RulePiece& kNull;

     private:
      // Empty piece means any
      QByteArray piece_;
      bool case_sensitive_ = false;
      // Key is 0 for any non-lit rules. Note, can have
      //  multiple values per char
      QHash<char, RulePiece> children_;

      // This is checked at the end for things like not-domain
      Info* rule_this_terminates_ = nullptr;
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

  struct ListMetadata {
    QString homepage;
    QString title;
    int expiration_hours = 0;
    QByteArray checksum;
    QString redirect;
    qlonglong version = 0;
    qlonglong rule_count = 0;
  };

  // Caller is responsible for deletion of these values
  static QList<Rule*> ParseRules(QTextStream* stream,
                                 int file_index,
                                 bool* parse_ok = nullptr);

  // This does not take ownership of any rules
  static ListMetadata GetMetadata(const QList<Rule*>& rules);

  QJsonObject RuleTree() const;

  bool AddRules(QTextStream* stream, int file_index);
  // This does not take ownership of any rules
  void AddRules(const QList<Rule*>& rules);

  StaticRule::Info* FindStaticRule(
      const QString& target_url,
      const QString& ref_url,
      StaticRule::RequestType request_type) const;

  StaticRule::Info* FindStaticRule(
      const QUrl& target_url,
      const QUrl& ref_url,
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

  StaticRule::Info* FindStaticRule(const StaticRule::MatchContext& ctx) const;
  StaticRule::Info* FindStaticRuleInRefHostHash(
      const StaticRule::MatchContext& ctx, const RefHostHash& hash) const;
  StaticRule::Info* FindStaticRuleInTargetHostHash(
      const StaticRule::MatchContext& ctx, const TargetHostHash& hash) const;
  StaticRule::Info* FindStaticRuleInRuleHash(
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
};

}  // namespace doogie

#endif  // DOOGIE_BLOCKER_RULES_H_
