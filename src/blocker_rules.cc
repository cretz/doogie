#include "blocker_rules.h"

#include <iterator>

namespace std {

inline uint qHash(const std::string& str) {
  return static_cast<uint>(std::hash<std::string>{}(str));
}

}  // namespace std

namespace doogie {

// Hash helpers
#ifdef USE_QHASH
#define ITER_KEY(_ITER) _ITER.key()
#define ITER_VAL(_ITER) _ITER.value()
#else
#define ITER_KEY(_ITER) _ITER->first
#define ITER_VAL(_ITER) _ITER->second
#endif

BlockerRules::Rule* BlockerRules::Rule::ParseRule(const QString& line,
                                                  int file_index,
                                                  int line_num) {
  if (line.isEmpty() || line.startsWith("[Adblock")) return nullptr;
  Rule* rule;
  if (line.startsWith("!")) {
    rule = CommentRule::ParseRule(line);
  } else if (line.contains("##")) {
    // TODO(cretz): Make a null result here try the static rule
    //  just in case there are two hashes in the URL or something.
    rule = CosmeticRule::ParseRule(line);
  } else {
    rule = StaticRule::ParseRule(line, file_index, line_num);
  }
  if (rule) {
    rule->file_index_ = file_index;
    rule->line_num_ = line_num;
  }
  return rule;
}

BlockerRules::CommentRule* BlockerRules::CommentRule::ParseRule(
    const QString& line) {
  if (!line.startsWith("! ")) return nullptr;
  auto ret = new CommentRule();
  ret->line_ = line.mid(2);
  return ret;
}

QString BlockerRules::CommentRule::MetadataKey() const {
  int colon_index = line_.indexOf(": ");
  if (colon_index == -1) return QString();
  return line_.left(colon_index);
}

QString BlockerRules::CommentRule::MetadataValue() const {
  int colon_index = line_.indexOf(": ");
  if (colon_index == -1) return QString();
  return line_.mid(colon_index + 2);
}

QString BlockerRules::StaticRule::FindResult::ToRuleString() const {
  // If it there is a target host, it's two pipes and then that
  QString ret;
  if (!target_host.isEmpty()) ret += QString("||") + target_host;
  for (int i = 0; i < pieces.size(); i++) {
    // If the string is already empty and we start w/ an asterisk
    //  it is implied.
    if (ret.isEmpty() && pieces[i] == "*") continue;
    // Ending asterisks are always implied
    if (i == pieces.size() - 1 && pieces[i] == "*") continue;
    // Otherwise, just append as normal
    ret += pieces[i];
  }
  // Add some non-default options if needed
  bool has_options = false;
  if (party != AnyParty) {
    has_options = true;
    ret += party == ThirdParty ? "$third-party" : "$first-party";
  }
  if (request_type != AllRequests) {
    ret += has_options ? "," : "$";
    has_options = true;
    ret += kRequestTypeToString.value(request_type);
  }
  if (!ref_host.isEmpty()) {
    ret += has_options ? "," : "$";
    has_options = true;
    ret += QString("domain=%1").arg(ref_host);
  }
  return ret;
}

BlockerRules::StaticRule::RulePiece::RulePiece() {
  id_ = id_counter_;
  id_counter_ += 256;
}

BlockerRules::StaticRule::RulePiece::RulePiece(const QByteArray& piece) {
  if (piece != "*") {
    // Squeeze the piece size if needed
    piece_ = piece;
    if (piece_.size() != piece_.capacity()) piece_.squeeze();
  }
  id_ = id_counter_;
  id_counter_ += 256;
}

void BlockerRules::StaticRule::RulePiece::AppendRule(
    StaticRule::AppendContext& ctx, int piece_index) {
  auto& piece = ctx.rule->Pieces()[piece_index];
  // As a shortcut, if we're the same piece and we have more indices,
  //  just go one deeper eagerly. This really only helps the root w/
  //  asterisks.
  if ((piece == piece_ || (piece == "*" && piece_.isNull())) &&
      piece_index < ctx.rule->Pieces().length() - 1) {
    AppendRule(ctx, piece_index + 1);
    return;
  }

  char first_chr = piece[0];
  if (first_chr == '*' || first_chr == '|' || first_chr == '^') {
    // This means the real first chr is 0 for lookup purposes
    first_chr = 0;
  }

  auto update_child = [&](RulePiece& child) {
    // If we're the last, terminate and leave. Otherwise go another deeper.
    if (piece_index == ctx.rule->Pieces().length() - 1 ||
        (piece_index == ctx.rule->Pieces().length() - 2 &&
         ctx.rule->Pieces()[piece_index + 1] == "*")) {
      child.rule_this_terminates_ = ctx.info;
    } else {
      child.AppendRule(ctx, piece_index + 1);
    }
  };

  // We check all the children to see if there are any we already match
  //  and if there are, we just add our next to their next
  auto& vec = (*ctx.piece_children)[id_ + first_chr];
  for (auto& child : vec) {
    if (child.piece_ == piece) {
      update_child(child);
      return;
    }
  }
  // Nope, need a new rule piece
  vec.emplace_back(piece);
  vec.back().case_sensitive_ = ctx.rule->case_sensitive_;
  update_child(vec.back());
}

BlockerRules::StaticRule::FindResult*
    BlockerRules::StaticRule::RulePiece::CheckMatch(
      const MatchContext& ctx, int curr_index) const {
  // Check if we match
  auto is_any = piece_.isEmpty();
  if (!is_any) {
    switch (piece_[0]) {
      case '|':
        // Start or end only
        if (curr_index != 0 &&
            curr_index != ctx.target_url.length()) return nullptr;
        break;
      case '^':
        // This matches only non-letter, non-digit, and not: _-.%
        // That means non-existent (i.e. end) also matches
        if (curr_index < ctx.target_url.length()) {
          auto url_ch = ctx.target_url[curr_index];
          if ((url_ch >= 'a' && url_ch <= 'z') ||
              (url_ch >= '0' && url_ch <= '9') ||
              url_ch == '_' || url_ch == '-' ||
              url_ch == '.' || url_ch == '%') {
            return nullptr;
          }
          curr_index++;
        }
        break;
      default:
        // Full piece must match
        for (int i = 0; i < piece_.length(); i++) {
          if (i + curr_index >= ctx.target_url.length()) return nullptr;
          auto piece_ch = piece_[i];
          auto url_ch = ctx.target_url[curr_index + i];
          if (piece_ch != url_ch) {
            // If it's not case-sensitive, we'll check the lower-case
            //  version too. Note, we can assume that case-insensitive
            //  pieces are all lowercase.
            if (case_sensitive_ ||
                url_ch < 'A' || url_ch > 'Z' || url_ch + 32 != piece_ch) {
              return nullptr;
            }
          }
        }
        curr_index += piece_.length();
    }
  }

  // If we matched a rule ourself, we're done
  if (rule_this_terminates_) {
    // Everything else is usually checked at the rule level, but
    //  we choose to check excluded file indexes, "excluded domains", and
    //  "excluded types" here...
    if (!ctx.ignored_file_indexes.contains(
          rule_this_terminates_->file_index) &&
        !rule_this_terminates_->not_request_types.test(ctx.request_type) &&
        !ctx.ref_hosts.intersects(rule_this_terminates_->not_ref_domains)) {
      auto result = new FindResult();
      result->info = *rule_this_terminates_;
      if (is_any) {
        result->pieces << "*";
      } else {
        result->pieces << piece_;
        // There is an implicit "any" at the end if not a pipe
        if (piece_[0] != '|') result->pieces << "*";
      }
      return result;
    }
  }

  // Try all char-0's (i.e. any and seps)
  FindResult* result = nullptr;
  PieceChildHash::const_iterator iter = ctx.piece_children->find(id_);
  if (iter != ctx.piece_children->cend()) {
    for (const auto& child : ITER_VAL(iter)) {
      result = child.CheckMatch(ctx, curr_index);
      if (result) break;
    }
  }

  // We have to try next string chars
  for (int i = curr_index; !result && i < ctx.target_url.length(); i++) {
    char url_ch = ctx.target_url[i];
    iter = ctx.piece_children->find(id_ + url_ch);
    if (iter != ctx.piece_children->cend()) {
      for (const auto& child : ITER_VAL(iter)) {
        result = child.CheckMatch(ctx, i);
        if (result) break;
      }
    }
    // If it's upper case, we also need to check the lower case char
    if (!result && url_ch >= 'A' && url_ch <= 'Z') {
      url_ch += 32;
      iter = ctx.piece_children->find(id_ + url_ch);
      if (iter != ctx.piece_children->cend()) {
        for (const auto& child : ITER_VAL(iter)) {
          result = child.CheckMatch(ctx, i);
          if (result) break;
        }
      }
    }
    // Wait, only ANY forces us to try each char, everything
    // else is a failure if it gets this far
    if (!is_any || result) break;
  }

  // If there is a result, we insert our piece
  if (result) result->pieces.prepend(piece_.isEmpty() ? "*" : piece_);
  return result;
}

void BlockerRules::StaticRule::RulePiece::RuleTree(
    QJsonObject* obj, const PieceChildHash* piece_children) const {
  auto piece = piece_.isEmpty() ? "*" : piece_;
  if (rule_this_terminates_) {
    obj->insert(piece, QString("Terminates for file %1 and line %2").
        arg(rule_this_terminates_->file_index).
        arg(rule_this_terminates_->line_num));
    return;
  }
  QJsonObject child;
  for (int i = 0; i < 256; i++) {
    PieceChildHash::const_iterator it = piece_children->find(id_ + i);
    if (it == piece_children->cend()) continue;
    QString key = i == 0 ? "special" : QString(static_cast<char>(i));
    auto child_obj = child[key].toObject();
    for (const auto& child : ITER_VAL(it)) {
      child.RuleTree(&child_obj, piece_children);
    }
    child[key] = child_obj;
  }
  obj->insert(piece, child);
}

quint64 BlockerRules::StaticRule::RulePiece::id_counter_ = 0;

BlockerRules::StaticRule* BlockerRules::StaticRule::ParseRule(
    const QString& line, int, int line_num) {
  if (line.isEmpty()) return nullptr;
  auto ret = new StaticRule();
  auto rule_bytes = line.toLatin1();

  // Parse all of the options
  int dollar = rule_bytes.lastIndexOf('$');
  if (dollar != -1) {
    for (const auto& option : rule_bytes.mid(dollar + 1).split(',')) {
      if (option.isEmpty()) continue;
      auto inv = option[0] == '~';
      auto& text = inv ? option.mid(1) : option;
      auto request_type = kStringToRequestType.value(text);
      if (request_type != AllRequests) {
        if (inv) {
          ret->not_request_types_ << request_type;
        } else {
          ret->request_types_ << request_type;
        }
      } else if (text == "third-party") {
        ret->request_party_ = inv ? FirstParty : ThirdParty;
      } else if (text.startsWith("domain=") && !inv) {
        for (auto& domain : text.mid(7).split('|')) {
          if (!domain.isEmpty()) {
            if (domain[0] == '~') {
              ret->not_ref_domains_ << domain.mid(1);
            } else {
              ret->ref_domains_ << domain;
            }
          }
        }
      } else if (text == "match-case" && !inv) {
        ret->case_sensitive_ = true;
      } else if (text == "collapse") {
        ret->collapse_option_ = inv ? NeverHide : AlwaysHide;
      } else {
        // TODO(cretz): sitekey, donottrack
        qDebug() << "Line" << line_num << "unrecognized option:" << option;
        return nullptr;
      }
    }
    rule_bytes = rule_bytes.left(dollar);
  }

  // Check whether exception
  if (rule_bytes.length() > 2 &&
      rule_bytes[0] == '@' && rule_bytes[1] == '@') {
    ret->exception_ = true;
    rule_bytes = rule_bytes.mid(2);
  }

  // Check whether we need to add implicit wildcards
  if (!rule_bytes.startsWith('|') && !rule_bytes.startsWith('*')) {
    rule_bytes.prepend('*');
  }
  if (!rule_bytes.endsWith('|') && !rule_bytes.endsWith('*')) {
    rule_bytes += '*';
  }

  // Check whether there's a start domain
  if (rule_bytes.length() > 2 &&
      rule_bytes[0] == '|' && rule_bytes[1] == '|') {
    // The begin domain name is everything until we reach an invalid
    //  hostname char or the end
    for (int i = 2; i < rule_bytes.length(); i++) {
      auto ch = rule_bytes[i];
      if ((ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') ||
          ch == '.' || ch == '-') {
        ret->target_domain_name_ += ch;
      } else {
        break;
      }
    }
    rule_bytes = rule_bytes.mid(2 + ret->target_domain_name_.length());
  }

  // Tokenize the rule
  auto curr_index = 0;
  while (curr_index < rule_bytes.length()) {
    int next_sep_index;
    for (next_sep_index = curr_index;
         next_sep_index < rule_bytes.length();
         next_sep_index++) {
      auto ch = rule_bytes[next_sep_index];
      if (ch == '|' || ch == '^' || ch == '*') break;
    }
    if (next_sep_index > curr_index) {
      auto piece = rule_bytes.mid(curr_index, next_sep_index - curr_index);
      // If this is not case sensitive, we need to lowercase our check
      if (!ret->case_sensitive_) piece = piece.toLower();
      ret->pieces_ << piece;
    }
    if (next_sep_index < rule_bytes.length()) {
      ret->pieces_ << rule_bytes.mid(next_sep_index, 1);
    }
    curr_index = next_sep_index + 1;
  }
  return ret;
}

const QHash<QByteArray, BlockerRules::StaticRule::RequestType>
    BlockerRules::StaticRule::kStringToRequestType = {
  { "all-requests", BlockerRules::StaticRule::RequestType::AllRequests },
  { "script", BlockerRules::StaticRule::RequestType::Script },
  { "image", BlockerRules::StaticRule::RequestType::Image },
  { "stylesheet", BlockerRules::StaticRule::RequestType::Stylesheet },
  { "object", BlockerRules::StaticRule::RequestType::Object },
  { "xmlhttprequest", BlockerRules::StaticRule::RequestType::XmlHttpRequest },
  { "object-subrequest",
    BlockerRules::StaticRule::RequestType::ObjectSubRequest },
  { "subdocument", BlockerRules::StaticRule::RequestType::SubDocument },
  { "ping", BlockerRules::StaticRule::RequestType::Ping },
  { "websocket", BlockerRules::StaticRule::RequestType::WebSocket },
  { "webrtc", BlockerRules::StaticRule::RequestType::WebRtc },
  { "document", BlockerRules::StaticRule::RequestType::Document },
  { "elemhide", BlockerRules::StaticRule::RequestType::ElemHide },
  { "generichide", BlockerRules::StaticRule::RequestType::GenericHide },
  { "genericblock", BlockerRules::StaticRule::RequestType::GenericBlock },
  { "popup", BlockerRules::StaticRule::RequestType::Popup },
  { "font", BlockerRules::StaticRule::RequestType::Font },
  { "media", BlockerRules::StaticRule::RequestType::Media },
  { "other", BlockerRules::StaticRule::RequestType::Other }
};

QHash<BlockerRules::StaticRule::RequestType, QByteArray> FlipRequestTypes() {
  QHash<BlockerRules::StaticRule::RequestType, QByteArray> ret;
  ret.reserve(BlockerRules::StaticRule::kStringToRequestType.size());
  for (auto& key : BlockerRules::StaticRule::kStringToRequestType.keys()) {
    ret[BlockerRules::StaticRule::kStringToRequestType[key]] = key;
  }
  return ret;
}
const QHash<BlockerRules::StaticRule::RequestType, QByteArray>
    BlockerRules::StaticRule::kRequestTypeToString = FlipRequestTypes();

BlockerRules::CosmeticRule* BlockerRules::CosmeticRule::ParseRule(
    const QString& /*line*/) {
  // TODO(cretz): this
  return nullptr;
}

QList<BlockerRules::Rule*> BlockerRules::ParseRules(QTextStream* stream,
                                                    int file_index,
                                                    bool* parse_ok) {
  QList<BlockerRules::Rule*> ret;
  int line_num = 0;
  while (!stream->atEnd()) {
    line_num++;
    const auto& line = stream->readLine();
    auto rule = Rule::ParseRule(line, file_index, line_num);
    if (rule) ret.append(rule);
  }
  // We intentionally choose not to check the embedded checksum here.
  // Anyone who can inject filters can change the checksum. We might consider
  //  returning the checksum so callers can check against an external source.
  if (parse_ok) *parse_ok = true;
  return ret;
}

BlockerRules::ListMetadata BlockerRules::GetMetadata(
    const QList<Rule*>& rules) {
  ListMetadata ret = {};
  for (const auto rule : rules) {
    auto comment = rule->AsComment();
    if (!comment) {
      ret.rule_count++;
      continue;
    }
    auto key = comment->MetadataKey();
    if (key.isEmpty()) continue;
    auto value = comment->MetadataValue();
    if (value.isEmpty()) continue;
    if (key == "Homepage") {
      ret.homepage = value;
    } else if (key == "Title") {
      ret.title = value;
    } else if (key == "Expires") {
      auto expires = value.split(' ');
      if (expires.size() < 2) continue;
      auto ok = false;
      auto amount = expires[0].toInt(&ok);
      if (!ok) continue;
      if (expires[1] == "days" || expires[1] == "day") {
        ret.expiration_hours = amount * 24;
      } else if (expires[1] == "hours" || expires[1] == "hour") {
        ret.expiration_hours = amount;
      }
    } else if (key == "Checksum") {
      ret.checksum = value.toLatin1();
    } else if (key == "Version") {
      auto ok = false;
      auto version = value.toLongLong(&ok);
      if (!ok) continue;
      ret.version = version;
    }
  }
  return ret;
}

BlockerRules::~BlockerRules() {
  // We need to delete all of the info pointers
  for (StaticRule::Info* info_ptr : info_ptrs_) delete info_ptr;
  info_ptrs_.clear();
}

QJsonObject BlockerRules::RuleTree() const {
  QJsonObject ret;
  ret["rules"] = RuleTreeForPartyHash(static_rules_);
  ret["exceptions"] = RuleTreeForPartyHash(static_rule_exceptions_);
  return ret;
}

bool BlockerRules::AddRules(QTextStream* stream, int file_index) {
  bool ok;
  auto rules = ParseRules(stream, file_index, &ok);
  if (!ok) return false;
  AddRules(rules);
  qDeleteAll(rules);
  return true;
}

void BlockerRules::AddRules(const QList<Rule*>& rules) {
  for (const auto rule : rules) {
    auto st = rule->AsStatic();
    if (st) AddStaticRule(st);
  }
  info_ptrs_.shrink_to_fit();
}

BlockerRules::StaticRule::FindResult* BlockerRules::FindStaticRule(
    const QString& target_url,
    const QString& ref_url,
    StaticRule::RequestType request_type,
    const QSet<int>& ignored_file_indexes) const {
  QUrl target_url_parsed(target_url, QUrl::StrictMode);
  QUrl ref_url_parsed(ref_url, QUrl::StrictMode);
  return FindStaticRule(target_url_parsed,
                        ref_url_parsed,
                        request_type,
                        ignored_file_indexes);
}

BlockerRules::StaticRule::FindResult* BlockerRules::FindStaticRule(
    const QUrl& target_url,
    const QUrl& ref_url,
    StaticRule::RequestType request_type,
    const QSet<int>& ignored_file_indexes) const {
  // We require URLs w/ schemes and hosts
  if (!target_url.isValid() || !ref_url.isValid() ||
      target_url.scheme().isEmpty() || ref_url.scheme().isEmpty() ||
      target_url.host().isEmpty() || ref_url.host().isEmpty()) {
    return nullptr;
  }

  // Get whether it is same origin or not
  auto url_origin = [](const QUrl& url) -> QString {
    return QString("%1://%2:%3").arg(url.scheme()).
        arg(url.host()).arg(url.port(url.scheme() == "http" ? 80 : 443));
  };
  auto same_origin =
      url_origin(target_url) == url_origin(ref_url);

  auto url_bytes = [](const QUrl& url) -> QByteArray {
    return url.toEncoded(QUrl::RemoveUserInfo | QUrl::FullyEncoded);
  };

  auto hosts = [](const QString& host) -> QSet<QByteArray> {
    QSet<QByteArray> ret;
    auto pieces = host.toLatin1().split('.');
    ret.reserve(pieces.length() - 1);
    auto curr = pieces[pieces.length() - 1];
    for (int i = pieces.length() - 2; i >= 0; i--) {
      curr.prepend('.');
      curr.prepend(pieces[i]);
      ret << curr;
    }
    return ret;
  };

  // Create context and run
  StaticRule::MatchContext ctx = {
    request_type,  // request_type
    same_origin ? StaticRule::RequestParty::FirstParty :
        StaticRule::RequestParty::ThirdParty,  // request_party
    url_bytes(target_url),  // target_url
    hosts(target_url.host(QUrl::FullyEncoded)),  // target_hosts
    target_url.scheme().length() + 3 +
      target_url.
        host(QUrl::FullyDecoded).length(),  // target_url_after_host_index
    url_bytes(ref_url),  // ref_url
    hosts(ref_url.host(QUrl::FullyEncoded)),  // ref_hosts
    &piece_children_,  // piece_children
    ignored_file_indexes  // ignored_file_indexes
  };
  return FindStaticRule(ctx);
}

BlockerRules::StaticRule::FindResult* BlockerRules::FindStaticRule(
    const StaticRule::MatchContext& ctx) const {
  // Always check exceptions first since we'd have to check em anyways.
  // Check the specific party then the general one.
  StaticRule::FindResult* result = nullptr;
  PartyOptionHash::const_iterator iter =
      static_rule_exceptions_.find(ctx.request_party);
  if (iter != static_rule_exceptions_.cend()) {
    result = FindStaticRuleInRefHostHash(ctx, ITER_VAL(iter));
    if (result) return nullptr;
  }
  iter = static_rule_exceptions_.find(StaticRule::AnyParty);
  if (iter != static_rule_exceptions_.cend()) {
    result = FindStaticRuleInRefHostHash(ctx, ITER_VAL(iter));
    if (result) return nullptr;
  }
  iter = static_rules_.find(ctx.request_party);
  if (iter != static_rules_.cend()) {
    result = FindStaticRuleInRefHostHash(ctx, ITER_VAL(iter));
    if (result) {
      result->party = ctx.request_party;
      return result;
    }
  }
  iter = static_rules_.find(StaticRule::AnyParty);
  if (iter != static_rules_.cend()) {
    result = FindStaticRuleInRefHostHash(ctx, ITER_VAL(iter));
  }
  return result;
}

BlockerRules::StaticRule::FindResult*
    BlockerRules::FindStaticRuleInRefHostHash(
      const StaticRule::MatchContext& ctx, const RefHostHash& hash) const {
  if (hash.empty()) return nullptr;
  // Specific host pieces first, then the rest
  for (const auto& host : ctx.ref_hosts) {
    RefHostHash::const_iterator iter = hash.find(host.toStdString());
    if (iter != hash.cend()) {
      auto result = FindStaticRuleInTargetHostHash(ctx, ITER_VAL(iter));
      if (result) {
        result->ref_host = host;
        return result;
      }
    }
  }
  RefHostHash::const_iterator iter = hash.find("");
  if (iter == hash.cend()) return nullptr;
  return FindStaticRuleInTargetHostHash(ctx, ITER_VAL(iter));
}

BlockerRules::StaticRule::FindResult*
    BlockerRules::FindStaticRuleInTargetHostHash(
      const StaticRule::MatchContext& ctx, const TargetHostHash& hash) const {
  if (hash.empty()) return nullptr;
  for (const auto& host : ctx.target_hosts) {
    TargetHostHash::const_iterator iter = hash.find(host.toStdString());
    if (iter != hash.cend()) {
      // We need a different context here for the sub host
      // TODO(cretz): Is this too slow creating this all the time?
      auto sub_host_ctx = ctx.WithTargetUrlChanged(
            ctx.target_url.mid(ctx.target_url_after_host_index));
      auto result = FindStaticRuleInRuleHash(sub_host_ctx, ITER_VAL(iter));
      if (result) {
        result->target_host = host;
        return result;
      }
    }
  }
  // Can use the regular context here
  TargetHostHash::const_iterator iter = hash.find("");
  if (iter == hash.cend()) return nullptr;
  return FindStaticRuleInRuleHash(ctx, ITER_VAL(iter));
}

BlockerRules::StaticRule::FindResult* BlockerRules::FindStaticRuleInRuleHash(
    const StaticRule::MatchContext& ctx, const RuleHash& hash) const {
  if (ctx.request_type != StaticRule::AllRequests) {
    RuleHash::const_iterator iter = hash.find(ctx.request_type);
    if (iter != hash.cend()) {
      auto result = ITER_VAL(iter).CheckMatch(ctx);
      if (result) {
        result->request_type = ctx.request_type;
        return result;
      }
    }
  }
  // Try the any-request one
  RuleHash::const_iterator iter = hash.find(StaticRule::AllRequests);
  if (iter == hash.cend()) return nullptr;
  return ITER_VAL(iter).CheckMatch(ctx);
}

void BlockerRules::AddStaticRule(StaticRule* rule) {
  // Create info that will be used inside rule and save pointer
  //  for later deletion
  StaticRule::AppendContext ctx = {};
  ctx.rule = rule;
  ctx.info = new StaticRule::Info();
  for (const auto t : ctx.rule->NotRequestTypes()) {
    ctx.info->not_request_types[t] = true;
  }
  if (!ctx.rule->NotRefDomains().isEmpty()) {
    ctx.info->not_ref_domains.reserve(ctx.rule->NotRefDomains().size());
    for (auto d : ctx.rule->NotRefDomains()) {
      ctx.info->not_ref_domains << d;
    }
  }
  ctx.info->file_index = ctx.rule->FileIndex();
  ctx.info->line_num = ctx.rule->LineNum();
  info_ptrs_.push_back(ctx.info);
  ctx.piece_children = &piece_children_;

  if (ctx.rule->Exception()) {
    AddStaticRuleToRefHostHash(ctx,
                               &static_rule_exceptions_[ctx.rule->ReqParty()]);
  } else {
    AddStaticRuleToRefHostHash(ctx,
                               &static_rules_[ctx.rule->ReqParty()]);
  }
}

void BlockerRules::AddStaticRuleToRefHostHash(StaticRule::AppendContext& ctx,
                                              RefHostHash* hash) {
  if (ctx.rule->RefDomains().isEmpty()) {
    AddStaticRuleToTargetHostHash(ctx, &(*hash)[""]);
  } else {
    for (const auto& ref_domain : ctx.rule->RefDomains()) {
      AddStaticRuleToTargetHostHash(ctx, &(*hash)[ref_domain.toStdString()]);
    }
  }
}

void BlockerRules::AddStaticRuleToTargetHostHash(
    StaticRule::AppendContext& ctx, TargetHostHash* hash) {
  if (ctx.rule->TargetDomainName().isEmpty()) {
    AddStaticRuleToRuleHash(ctx, &(*hash)[""]);
  } else {
    AddStaticRuleToRuleHash(
          ctx, &(*hash)[ctx.rule->TargetDomainName().toStdString()]);
  }
}

void BlockerRules::AddStaticRuleToRuleHash(StaticRule::AppendContext& ctx,
                                           RuleHash* hash) {
  if (ctx.rule->RequestTypes().isEmpty()) {
    (*hash)[StaticRule::AllRequests].AppendRule(ctx);
  } else {
    for (const auto request_type : ctx.rule->RequestTypes()) {
      (*hash)[request_type].AppendRule(ctx);
    }
  }
}

QJsonObject BlockerRules::RuleTreeForPartyHash(
    const PartyOptionHash& hash) const {
  QJsonObject ret;
  for (auto iter = hash.cbegin(); iter != hash.cend(); iter++) {
    QString key = ITER_KEY(iter) == StaticRule::AnyParty ?
        "any party" : (ITER_KEY(iter) == StaticRule::ThirdParty ?
            "third party" : "first party");
    ret[key] = RuleTreeForRefHostHash(ITER_VAL(iter));
  }
  return ret;
}

QJsonObject BlockerRules::RuleTreeForRefHostHash(
    const RefHostHash& hash) const {
  QJsonObject ret;
  for (auto iter = hash.cbegin(); iter != hash.cend(); iter++) {
    QString key = ITER_KEY(iter).empty() ?
          "any ref host" : QString::fromStdString(ITER_KEY(iter));
    ret[key] = RuleTreeForTargetHostHash(ITER_VAL(iter));
  }
  return ret;
}

QJsonObject BlockerRules::RuleTreeForTargetHostHash(
    const TargetHostHash& hash) const {
  QJsonObject ret;
  for (auto iter = hash.cbegin(); iter != hash.cend(); iter++) {
    QString key = ITER_KEY(iter).empty() ?
          "any target host" : QString::fromStdString(ITER_KEY(iter));
    ret[key] = RuleTreeForRuleHash(ITER_VAL(iter));
  }
  return ret;
}

QJsonObject BlockerRules::RuleTreeForRuleHash(
    const RuleHash& hash) const {
  QJsonObject ret;
  QHash<QByteArray, StaticRule::RequestType>::const_iterator iter =
      StaticRule::kStringToRequestType.constBegin();
  while (iter != StaticRule::kStringToRequestType.constEnd()) {
    RuleHash::const_iterator citer = hash.find(iter.value());
    if (citer != hash.cend()) {
      QJsonObject piece_json;
      ITER_VAL(citer).RuleTree(&piece_json, &piece_children_);
      ret[iter.key()] = piece_json;
    }
    ++iter;
  }
  return ret;
}

}  // namespace doogie
