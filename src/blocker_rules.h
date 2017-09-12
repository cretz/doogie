#ifndef DOOGIE_BLOCKER_RULES_H_
#define DOOGIE_BLOCKER_RULES_H_

// This was determined the best to use after some tests with other map
//  implementations. The only other one of note was sparsepp. While it
//  appeared to use similar mem, it was a good bit slower on initial
//  creation. Not having it helps keep third-party libs to a minimum.
#define USE_QHASH 1

#include <QtWidgets>
#include <bitset>
#include <memory>
#include <string>
#include <vector>
// #include <sparsepp/spp.h>
// #include <unordered_map>
// #include <hopscotch_map.h>
// #include <sparsehash/sparse_hash_map>

namespace doogie {

// Class (and nested classes) for parsing and handling of blocker
// filter lists.
class BlockerRules {
 public:
#ifdef USE_QHASH
  template<typename K, typename V> using Hash = QHash<K, V>;
#else
  template<typename K, typename V> using Hash = spp::sparse_hash_map<K, V>;
  // template<typename K, typename V> using Hash = std::unordered_map<K, V>;
#endif

  class CommentRule;
  class StaticRule;
  class CosmeticRule;
  class Rule {
   public:
    static Rule* ParseRule(const QString& line,
                           int file_index,
                           int line_num);

    virtual ~Rule() {}
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
    class RulePiece;
    // Keyed by piece ID + starting char
    typedef Hash<quint64, std::vector<RulePiece>> PieceChildHash;

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
                 target_url_after_host_index, ref_url,
                 ref_hosts, piece_children, ignored_file_indexes };
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

      const PieceChildHash* piece_children;
      const QSet<int> ignored_file_indexes;
    };

    struct Info {
      // We choose vectors here because of size constraints
      std::bitset<Other + 1> not_request_types;
      QSet<QByteArray> not_ref_domains;
      int file_index = -1;
      int line_num = -1;
    };

    struct AppendContext {
      StaticRule* rule;
      Info* info;
      PieceChildHash* piece_children;
    };

    struct FindResult {
      QString ToRuleString() const;

      QList<QByteArray> pieces;
      Info info;
      RequestParty party = AnyParty;
      QString ref_host;
      QString target_host;
      RequestType request_type = AllRequests;
    };

    class RulePiece {
     public:
      RulePiece();
      explicit RulePiece(const QByteArray& piece);

      Info* RuleThisTerminates() const { return rule_this_terminates_; }
      void AppendRule(AppendContext& ctx,  // NOLINT(runtime/references)
                      int piece_index = 0);

      // Caller is responsible for deletion of result.
      FindResult* CheckMatch(const MatchContext& ctx,
                             int curr_index = 0) const;

      void RuleTree(QJsonObject* obj,
                    const PieceChildHash* piece_children) const;

     private:
      static quint64 id_counter_;

      typedef Hash<char, std::vector<RulePiece>> ChildMap;

      // Empty piece means any
      quint64 id_;
      QByteArray piece_;
      bool case_sensitive_ = false;

      // This is checked at the end for things like not-domain. We know this
      //  pointer is frequently copied, but it's not owned by us.
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

    static const QHash<QByteArray, RequestType> kStringToRequestType;
    static const QHash<RequestType, QByteArray> kRequestTypeToString;

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

  ~BlockerRules();

  QJsonObject RuleTree() const;

  bool AddRules(QTextStream* stream, int file_index);
  // This does not take ownership of any rules
  void AddRules(const QList<Rule*>& rules);

  // Caller is responsible for deletion of result.
  StaticRule::FindResult* FindStaticRule(
      const QString& target_url,
      const QString& ref_url,
      StaticRule::RequestType request_type,
      const QSet<int>& ignored_file_indexes = {}) const;

  // Caller is responsible for deletion of result.
  StaticRule::FindResult* FindStaticRule(
      const QUrl& target_url,
      const QUrl& ref_url,
      StaticRule::RequestType request_type,
      const QSet<int>& ignored_file_indexes = {}) const;

 private:
  // Keyed by the request type or AllRequests if it applies to all.
  typedef Hash<StaticRule::RequestType, StaticRule::RulePiece> RuleHash;
  // Keyed by the target domain. The rules within expect to have the scheme
  //  and the domain removed. If the rule is not domain specific, it's in
  //  the empty string form and the domain does not need to be removed.
  typedef Hash<std::string, RuleHash> TargetHostHash;
  // Keyed by the ref domain. Non-ref-domain-specific rules are in the empty
  //  string key.
  typedef Hash<std::string, TargetHostHash> RefHostHash;
  // Keyed by whether the rule is for first party, third party, or both
  typedef Hash<StaticRule::RequestParty, RefHostHash> PartyOptionHash;

  StaticRule::FindResult* FindStaticRule(
      const StaticRule::MatchContext& ctx) const;
  StaticRule::FindResult* FindStaticRuleInRefHostHash(
      const StaticRule::MatchContext& ctx, const RefHostHash& hash) const;
  StaticRule::FindResult* FindStaticRuleInTargetHostHash(
      const StaticRule::MatchContext& ctx, const TargetHostHash& hash) const;
  StaticRule::FindResult* FindStaticRuleInRuleHash(
      const StaticRule::MatchContext& ctx, const RuleHash& hash) const;

  void AddStaticRule(StaticRule* rule);
  void AddStaticRuleToRefHostHash(
      StaticRule::AppendContext& ctx,  // NOLINT(runtime/references)
      RefHostHash* hash);
  void AddStaticRuleToTargetHostHash(
      StaticRule::AppendContext& ctx,  // NOLINT(runtime/references)
      TargetHostHash* hash);
  void AddStaticRuleToRuleHash(
      StaticRule::AppendContext& ctx,  // NOLINT(runtime/references)
      RuleHash* hash);

  QJsonObject RuleTreeForPartyHash(const PartyOptionHash& hash) const;
  QJsonObject RuleTreeForRefHostHash(const RefHostHash& hash) const;
  QJsonObject RuleTreeForTargetHostHash(const TargetHostHash& hash) const;
  QJsonObject RuleTreeForRuleHash(const RuleHash& hash) const;

  PartyOptionHash static_rules_;
  PartyOptionHash static_rule_exceptions_;
  std::vector<StaticRule::Info*> info_ptrs_;
  StaticRule::PieceChildHash piece_children_;
};

}  // namespace doogie

#endif  // DOOGIE_BLOCKER_RULES_H_
