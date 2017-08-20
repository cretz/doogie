#include "blocker_rules.h"

namespace doogie {

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
  if (line.startsWith("! ")) return nullptr;
  auto ret = new CommentRule();
  ret->line_ = line.mid(2);
  return ret;
}

BlockerRules::StaticRule::RulePiece::RulePiece() { }

BlockerRules::StaticRule::RulePiece::RulePiece(const QByteArray& piece) {
  if (piece != "*") {
    // Squeeze the piece size if needed
    piece_ = piece;
    if (piece_.size() != piece_.capacity()) piece_.squeeze();
  }
}

BlockerRules::StaticRule::RulePiece::~RulePiece() {
  if (rule_this_terminates_) {
    delete rule_this_terminates_;
    rule_this_terminates_ = nullptr;
  }
}

void BlockerRules::StaticRule::RulePiece::AppendRule(StaticRule* rule,
                                                     int piece_index) {
  auto& piece = rule->Pieces()[piece_index];
  // As a shortcut, if we're the same piece and we have more indices,
  //  just go one deeper eagerly. This really only helps the root w/
  //  asterisks.
  if ((piece == piece_ || (piece == "*" && piece_.isNull())) &&
      piece_index < rule->Pieces().length() - 1) {
    AppendRule(rule, piece_index + 1);
    return;
  }

  char first_chr = piece[0];
  if (first_chr == '*' || first_chr == '|' || first_chr == '^') {
    // This means the real first chr is 0 for lookup purposes
    first_chr = 0;
  }

  auto update_child = [&](RulePiece& child) {
    // If we're the last, terminate and leave
    if (piece_index == rule->Pieces().length() - 1 ||
        (piece_index == rule->Pieces().length() - 2 &&
         rule->Pieces()[piece_index + 1] == "*")) {
      if (child.rule_this_terminates_) delete child.rule_this_terminates_;
      child.rule_this_terminates_ = new Info();
      for (const auto t : rule->NotRequestTypes()) {
        child.rule_this_terminates_->not_request_types[t] = true;
      }
      if (!rule->NotRefDomains().isEmpty()) {
        child.rule_this_terminates_->not_ref_domains.reserve(
              rule->NotRefDomains().size());
        for (auto d : rule->NotRefDomains()) {
          child.rule_this_terminates_->not_ref_domains << d;
        }
      }
      child.rule_this_terminates_->file_index = rule->FileIndex();
      child.rule_this_terminates_->line_num = rule->LineNum();
      return;
    }
    // Otherwise, go another deeper
    child.AppendRule(rule, piece_index + 1);
  };

  // We check all the children to see if there are any we already match
  //  and if there are, we just add our next to their next
  QHash<char, RulePiece>::iterator iter = children_.find(first_chr);
  while (iter != children_.end() && iter.key() == first_chr) {
    if (iter.value().piece_ == piece) {
      update_child(iter.value());
      return;
    }
    ++iter;
  }
  RulePiece child(piece);
  child.case_sensitive_ = rule->case_sensitive_;
  children_.reserve(children_.size() + 1);
  iter = children_.insertMulti(first_chr, child);
  update_child(iter.value());
}

const BlockerRules::StaticRule::RulePiece*
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
              url_ch == '_' || url_ch == '-' || url_ch == '.' || url_ch == '%') {
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
    //  we choose to check "excluded domains" and "excluded types" here...
    if (!rule_this_terminates_->not_request_types.test(ctx.request_type) &&
        !ctx.ref_hosts.intersects(rule_this_terminates_->not_ref_domains)) {
      return this;
    }
  }

  // Try all char-0's (i.e. any and seps)
  QHash<char, RulePiece>::const_iterator iter = children_.constFind(0);
  while (iter != children_.constEnd() && iter.key() == 0) {
    auto ret = iter.value().CheckMatch(ctx, curr_index);
    if (ret) return ret;
    ++iter;
  }

  // We have to try next string chars
  for (int i = curr_index; i < ctx.target_url.length(); i++) {
    char url_ch = ctx.target_url[i];
    iter = children_.constFind(url_ch);
    while (iter != children_.constEnd() && iter.key() == url_ch) {
      auto ret = iter.value().CheckMatch(ctx, i);
      if (ret) return ret;
      ++iter;
    }
    // If it's upper case, we also need to check the lower case char
    if (url_ch >= 'A' && url_ch <= 'Z') {
      url_ch += 32;
      iter = children_.constFind(url_ch);
      while (iter != children_.constEnd() && iter.key() == url_ch) {
        auto ret = iter.value().CheckMatch(ctx, i);
        if (ret) return ret;
        ++iter;
      }
    }
    // Wait, only ANY forces us to try each char, everything
    // else is a failure if it gets this far
    if (!is_any) break;
  }
  return nullptr;
}

void BlockerRules::StaticRule::RulePiece::RuleTree(QJsonObject* obj) const {
  auto piece = piece_.isEmpty() ? "*" : piece_;
  if (rule_this_terminates_) {
    obj->insert(piece, QString("Terminates for file %1 and line %2").
        arg(rule_this_terminates_->file_index).
        arg(rule_this_terminates_->line_num));
    return;
  }
  QJsonObject child;
  QHash<char, RulePiece>::const_iterator i = children_.constBegin();
  while (i != children_.constEnd()) {
    QString key = i.key() == 0 ? "special" : QString(i.key());
    auto child_obj = child[key].toObject();
    i.value().RuleTree(&child_obj);
    child[key] = child_obj;
    ++i;
  }
  obj->insert(piece, child);
}

const BlockerRules::StaticRule::RulePiece&
  BlockerRules::StaticRule::RulePiece::kNull =
    BlockerRules::StaticRule::RulePiece();

BlockerRules::StaticRule* BlockerRules::StaticRule::ParseRule(
    const QString& line, int, int line_num) {
  if (line.isEmpty()) return nullptr;
  auto ret = new StaticRule();
  auto& rule_bytes = line.toLatin1();

  // Parse all of the options
  int dollar = rule_bytes.lastIndexOf('$');
  if (dollar != -1) {
    for (const auto& option : rule_bytes.mid(dollar + 1).split(',')) {
      if (option.isEmpty()) continue;
      auto inv = option[0] == '~';
      auto& text = inv ? option.mid(1) : option;
      auto request_type = kRequestTypeStrings.value(text);
      if (request_type != AllRequests) {
        if (inv) {
          ret->not_request_types_ << request_type;
        } else {
          ret->request_types_ << request_type;
        }
      } else if (text == "third-party") {
        ret->request_party_ = inv ? FirstParty : ThirdParty;
      } else if (text.startsWith("domain=") && !inv) {
        for (const auto& domain : text.mid(7).split('|')) {
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
      if ((ch >= 'a' && ch <= 'z') || ch == '.' || ch == '-') {
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
    BlockerRules::StaticRule::kRequestTypeStrings = {
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

BlockerRules::CosmeticRule* BlockerRules::CosmeticRule::ParseRule(
    const QString& line) {
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
    auto& line = stream->readLine();
    auto rule = Rule::ParseRule(line, file_index, line_num);
    if (rule) ret.append(rule);
  }
  if (parse_ok) {
    // TODO(cretz): checksum
    *parse_ok = true;
  }
  return ret;
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
}

BlockerRules::StaticRule::Info* BlockerRules::FindStaticRule(
    const QString& target_url,
    const QString& ref_url,
    StaticRule::RequestType request_type) const {
  // Url-ify both
  QUrl target_url_parsed(target_url, QUrl::StrictMode);
  QUrl ref_url_parsed(ref_url, QUrl::StrictMode);
  // We require URLs w/ schemes and hosts
  if (!target_url_parsed.isValid() ||
      !ref_url_parsed.isValid() ||
      target_url_parsed.scheme().isEmpty() ||
      ref_url_parsed.scheme().isEmpty() ||
      target_url_parsed.host().isEmpty() ||
      ref_url_parsed.host().isEmpty()) {
    return nullptr;
  }

  // Get whether it is same origin or not
  auto url_origin = [](const QUrl& url) -> QString {
    return QString("%1://%2:%3").arg(url.scheme()).
        arg(url.host()).arg(url.port(url.scheme() == "http" ? 80 : 443));
  };
  auto same_origin =
      url_origin(target_url_parsed) == url_origin(ref_url_parsed);

  auto url_bytes = [](const QUrl& url) -> QByteArray {
    return url.toEncoded(QUrl::RemoveUserInfo | QUrl::FullyEncoded);
  };

  auto hosts = [](const QString& host) -> QSet<QByteArray> {
    QSet<QByteArray> ret;
    auto& pieces = host.toLatin1().split('.');
    ret.reserve(pieces.length() - 1);
    auto& curr = pieces[pieces.length() - 1];
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
    url_bytes(target_url_parsed),  // target_url
    hosts(target_url_parsed.host(QUrl::FullyEncoded)),  // target_hosts
    target_url_parsed.scheme().length() + 3 +
      target_url_parsed.
        host(QUrl::FullyDecoded).length(),  // target_url_after_host_index
    url_bytes(ref_url_parsed),  // ref_url
    hosts(ref_url_parsed.host(QUrl::FullyEncoded))  // ref_hosts
  };
  return FindStaticRule(ctx);
}

BlockerRules::StaticRule::Info* BlockerRules::FindStaticRule(
    const StaticRule::MatchContext& ctx) const {
  // Always check exceptions first since we'd have to check em anyways.
  // Check the specific party then the general one.
  auto rule = FindStaticRuleInRefHostHash(
      ctx, static_rule_exceptions_.value(ctx.request_party));
  if (rule) return nullptr;
  rule = FindStaticRuleInRefHostHash(
      ctx, static_rule_exceptions_.value(StaticRule::AnyParty));
  if (rule) return nullptr;
  rule = FindStaticRuleInRefHostHash(
      ctx, static_rules_.value(ctx.request_party));
  if (rule) return rule;
  rule = FindStaticRuleInRefHostHash(
      ctx, static_rules_.value(StaticRule::AnyParty));
  return rule;
}

BlockerRules::StaticRule::Info* BlockerRules::FindStaticRuleInRefHostHash(
    const StaticRule::MatchContext& ctx, const RefHostHash& hash) const {
  if (hash.isEmpty()) return nullptr;
  // Specific host pieces first, then the rest
  for (const auto& host : ctx.ref_hosts) {
    auto& to_check = hash.value(host);
    if (!to_check.isEmpty()) {
      auto rule = FindStaticRuleInTargetHostHash(ctx, to_check);
      if (rule) return rule;
    }
  }
  auto& to_check = hash.value("");
  if (to_check.isEmpty()) return nullptr;
  return FindStaticRuleInTargetHostHash(ctx, to_check);
}

BlockerRules::StaticRule::Info* BlockerRules::FindStaticRuleInTargetHostHash(
    const StaticRule::MatchContext& ctx, const TargetHostHash& hash) const {
  if (hash.isEmpty()) return nullptr;
  for (const auto& host : ctx.target_hosts) {
    auto& to_check = hash.value(host);
    if (!to_check.isEmpty()) {
      // TODO(cretz): Is this too slow creating this all the time?
      auto sub_host_ctx = ctx.WithTargetUrlChanged(
            ctx.target_url.mid(ctx.target_url_after_host_index));
      auto rule = FindStaticRuleInRuleHash(sub_host_ctx, to_check);
      if (rule) return rule;
    }
  }
  auto& to_check = hash.value("");
  if (to_check.isEmpty()) return nullptr;
  // Can use the regular context here
  return FindStaticRuleInRuleHash(ctx, to_check);
}

BlockerRules::StaticRule::Info* BlockerRules::FindStaticRuleInRuleHash(
    const StaticRule::MatchContext& ctx, const RuleHash& hash) const {
  if (ctx.request_type != StaticRule::AllRequests) {
    auto& to_check = hash.value(ctx.request_type, StaticRule::RulePiece::kNull);
    if (!to_check.IsNull()) {
      auto piece = to_check.CheckMatch(ctx);
      if (piece) return piece->RuleThisTerminates();
    }
  }
  // Try the any request one
  auto& to_check = hash.value(StaticRule::AllRequests,
                             StaticRule::RulePiece::kNull);
  if (to_check.IsNull()) return nullptr;
  auto piece = to_check.CheckMatch(ctx);
  if (piece) return piece->RuleThisTerminates();
  return nullptr;
}

void BlockerRules::AddStaticRule(StaticRule* rule) {
  if (rule->Exception()) {
    AddStaticRuleToRefHostHash(rule,
                               &static_rule_exceptions_[rule->ReqParty()]);
  } else {
    AddStaticRuleToRefHostHash(rule, &static_rules_[rule->ReqParty()]);
  }
}

void BlockerRules::AddStaticRuleToRefHostHash(StaticRule* rule,
                                              RefHostHash* hash) {
  if (rule->RefDomains().isEmpty()) {
    AddStaticRuleToTargetHostHash(rule, &(*hash)[""]);
  } else {
    for (const auto& ref_domain : rule->RefDomains()) {
      AddStaticRuleToTargetHostHash(rule, &(*hash)[ref_domain]);
    }
  }
}

void BlockerRules::AddStaticRuleToTargetHostHash(StaticRule* rule,
                                                 TargetHostHash* hash) {
  if (rule->TargetDomainName().isEmpty()) {
    AddStaticRuleToRuleHash(rule, &(*hash)[""]);
  } else {
    AddStaticRuleToRuleHash(rule, &(*hash)[rule->TargetDomainName()]);
  }
}

void BlockerRules::AddStaticRuleToRuleHash(StaticRule* rule,
                                           RuleHash* hash) {
  if (rule->RequestTypes().isEmpty()) {
    auto& piece = (*hash)[StaticRule::AllRequests];
    piece.AppendRule(rule);
  } else {
    for (const auto request_type : rule->RequestTypes()) {
      auto& piece = (*hash)[request_type];
      piece.AppendRule(rule);
    }
  }
}

QJsonObject BlockerRules::RuleTreeForPartyHash(
    const PartyOptionHash& hash) const {
  QJsonObject ret;
  PartyOptionHash::const_iterator iter = hash.constBegin();
  while (iter != hash.constEnd()) {
    QString key = iter.key() == StaticRule::AnyParty ? "any party" :
        (iter.key() == StaticRule::ThirdParty ? "third party" : "first party");
    ret[key] = RuleTreeForRefHostHash(iter.value());
    ++iter;
  }
  return ret;
}

QJsonObject BlockerRules::RuleTreeForRefHostHash(
    const RefHostHash& hash) const {
  QJsonObject ret;
  RefHostHash::const_iterator iter = hash.constBegin();
  while (iter != hash.constEnd()) {
    QByteArray key = iter.key().isEmpty() ? "any ref host" : iter.key();
    ret[key] = RuleTreeForTargetHostHash(iter.value());
    ++iter;
  }
  return ret;
}

QJsonObject BlockerRules::RuleTreeForTargetHostHash(
    const TargetHostHash& hash) const {
  QJsonObject ret;
  TargetHostHash::const_iterator iter = hash.constBegin();
  while (iter != hash.constEnd()) {
    QByteArray key = iter.key().isEmpty() ? "any target host" : iter.key();
    ret[key] = RuleTreeForRuleHash(iter.value());
    ++iter;
  }
  return ret;
}

QJsonObject BlockerRules::RuleTreeForRuleHash(
    const RuleHash& hash) const {
  QJsonObject ret;
  QHash<QByteArray, StaticRule::RequestType>::const_iterator iter =
      StaticRule::kRequestTypeStrings.constBegin();
  while (iter != StaticRule::kRequestTypeStrings.constEnd()) {
    auto& piece = hash.value(iter.value(), StaticRule::RulePiece::kNull);
    if (!piece.IsNull()) {
      QJsonObject piece_json;
      piece.RuleTree(&piece_json);
      ret[iter.key()] = piece_json;
    }
    ++iter;
  }
  return ret;
}

}  // namespace doogie
