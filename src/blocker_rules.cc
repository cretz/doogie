#include "blocker_rules.h"

namespace doogie {

BlockerRules::Rule BlockerRules::Rule::ParseRule(const QString& line,
                                                 int file_index,
                                                 int line_num) {
  auto trimmed = line.trimmed();
  Rule rule;
  if (trimmed.startsWith("!")) {
    rule = CommentRule::ParseRule(line);
  } else if (trimmed.startsWith("##")) {
    rule = CosmeticRule::ParseRule(line);
  } else if (!trimmed.isEmpty()) {
    rule = StaticRule::ParseRule(line, file_index, line_num);
  }
  rule.file_index_ = file_index;
  rule.line_num_ = line_num;
  return rule;
}

BlockerRules::CommentRule BlockerRules::CommentRule::ParseRule(
    const QString& line) {
  CommentRule ret;
  if (line.startsWith("! ")) ret.line_ = line.mid(2);
  return ret;
}

BlockerRules::CommentRule::CommentRule() { }

BlockerRules::StaticRule::RulePiece::RulePiece() { }

BlockerRules::StaticRule::RulePiece::RulePiece(const QByteArray& piece)
  : piece_(piece), match_(Lit) { }

BlockerRules::StaticRule::RulePiece::RulePiece(RuleMatch match)
  : match_(match) { }

const BlockerRules::StaticRule::RulePiece*
  BlockerRules::StaticRule::RulePiece::Match(
    const MatchContext& ctx, int curr_index) const {
  if (match_ == None) return nullptr;
  char ch;
  // Check if we match
  if (match_ == Lit) {
    for (int i = 0; i < piece_.length(); i++) {
      if (i >= ctx.url.length() || ctx.url.at(i) != piece_.at(i)) {
        return nullptr;
      }
    }
    curr_index += piece_.length();
  } else if (match_ == SingleSep) {
    // This matches only non-letter, non-digit, and not: _-.%
    // That means non-existent (i.e. end) also matches
    if (curr_index < ctx.url.length()) {
      ch = ctx.url.at(curr_index);
      if ((ch >= 'a' && ch <= 'z') ||
          (ch >= '0' && ch <= '9') ||
          ch == '_' || ch == '-' || ch == '.' || ch == '%') {
        return nullptr;
      }
      curr_index++;
    }
  } else if (match_ == MustEnd) {
    if (curr_index != ctx.url.length()) return nullptr;
  } else if (match_ == MustStart) {
    if (curr_index != 0) return nullptr;
  }

  // If we matched and we have a rule pos ourself, we're done
  if (file_index_ >= 0) return this;

  // Try all char-0's (i.e. any and seps)
  QHash<char, RulePiece>::const_iterator iter = children_.constFind(0);
  while (iter != children_.constEnd()) {
    auto ret = iter.value().Match(ctx, curr_index);
    if (ret) return ret;
    ++iter;
  }

  // We have to try next string chars
  for (int i = curr_index; i < ctx.url.length(); i++) {
    ch = ctx.url.at(i);
    iter = children_.constFind(ch);
    while (iter != children_.constEnd()) {
      auto ret = iter.value().Match(ctx, curr_index + i);
      if (ret) return ret;
      ++iter;
    }
    // Wait, only ANY forces us to try each char, everything
    // else is a failure if it gets this far
    if (match_ != Any) break;
  }
  return nullptr;
}

void BlockerRules::StaticRule::RulePiece::AddAsChildOf(RulePiece* parent) {
  parent->children_.insertMulti(piece_.isEmpty() ? 0 : piece_.at(0), *this);
}

BlockerRules::StaticRule BlockerRules::StaticRule::ParseRule(
    const QString& line, int file_index, int line_num) {

  StaticRule ret;
  auto rule_bytes = line.toLatin1();
  if (rule_bytes.isEmpty() || rule_bytes[0] == '!') return ret;
  int curr_index = 0;

  RulePiece* curr_piece = &ret.root_piece_;
  auto add_child_piece = [&curr_piece](RulePiece& new_piece) {
    new_piece.AddAsChildOf(curr_piece);
    curr_piece = &new_piece;
  };

  QByteArray option_bytes;
  int dollar = rule_bytes.lastIndexOf('$');
  if (dollar != -1) {
    option_bytes = rule_bytes.mid(dollar + 1);
    rule_bytes = rule_bytes.left(dollar);
  }

  QVector<QByteArray> tokens;
  // Tokenize the rule
  while (curr_index < rule_bytes.length()) {
    int next_sep_index;
    for (next_sep_index = curr_index;
         next_sep_index < rule_bytes.length();
         next_sep_index++) {
      auto ch = rule_bytes[next_sep_index];
      if (ch == '|' || ch == '^' || ch == '*') break;
    }
    if (next_sep_index > curr_index) {
      tokens << rule_bytes.mid(curr_index, next_sep_index);
    }
    if (next_sep_index < rule_bytes.length()) {
      tokens << rule_bytes.mid(next_sep_index, 1);
    }
    curr_index = next_sep_index + 1;
  }

  // Now we can test pieces...curr_index is now the vector index
  curr_index = 0;
  while (curr_index < tokens.length()) {
    auto token = tokens.at(curr_index);
    switch (token.at(0)) {
      case '|':
        if (curr_index == 0) {
          if (tokens.length() > 1 && tokens.at(1) == "|") {
            ret.starts_with_domain_ = true;
            curr_index++;
          }
          add_child_piece(RulePiece(MustStart));
        } else {
          add_child_piece(RulePiece(MustEnd));
        }
        break;
      case '^':
        // Have to add implicit any
        if (curr_index == 0) add_child_piece(RulePiece(Any));
        add_child_piece(RulePiece(SingleSep));
        break;
      case '*':
        add_child_piece(RulePiece(SingleSep));
        break;
      default:
        if (curr_index == 0) add_child_piece(RulePiece(Any));
        add_child_piece(RulePiece(token));
    }
    curr_index++;
  }

  curr_piece->SetAsTerminator(file_index, line_num);

  return ret;
}

BlockerRules::StaticRule::StaticRule() { }

BlockerRules::CosmeticRule BlockerRules::CosmeticRule::ParseRule(
    const QString& line) {
  // TODO(cretz): this
  return CosmeticRule();
}

QList<BlockerRules::Rule> BlockerRules::ParseRules(QTextStream* stream,
                                                   int file_index) {
  QList<BlockerRules::Rule> ret;
  int line_num = 0;
  while (!stream->atEnd()) {
    line_num++;
    auto rule = Rule::ParseRule(stream->readLine(), file_index, line_num);
    if (!rule.IsNull()) ret.append(rule);
  }
  return ret;
}

}  // namespace doogie
