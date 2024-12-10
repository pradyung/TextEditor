#include <ncurses.h>
#include <cctype>
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>
#include <iostream>

#define cut(str, position) str.substr(std::min((int)str.size(), e.colOffset), e.maxX - maxLineNumberLength - 1 - position).c_str()
#define DEFAULT_BLACK -1

enum Colors
{
  WHITE,
  CYAN,
  RED,
  GREEN,
  YELLOW,
  BLUE,
  MAGENTA,
  CYAN_BACK,
};

enum Highlights
{
  LINE_NUMBER = COLOR_PAIR(CYAN) | A_BOLD,
  MESSAGE = COLOR_PAIR(GREEN) | A_BOLD,
  DIRECTIVE = COLOR_PAIR(RED),
  STRING = COLOR_PAIR(GREEN),
  KEYWORD = COLOR_PAIR(YELLOW),
  NUMBER = COLOR_PAIR(MAGENTA),
  COMMENT = COLOR_PAIR(CYAN),
  BRACKET_LEVEL_1 = COLOR_PAIR(RED),
  BRACKET_LEVEL_2 = COLOR_PAIR(YELLOW),
  BRACKET_LEVEL_3 = COLOR_PAIR(GREEN),
  FIND = COLOR_PAIR(CYAN_BACK)
};

int BRACKET_HIGHLIGHTS[] = {
    BRACKET_LEVEL_1,
    BRACKET_LEVEL_2,
    BRACKET_LEVEL_3};

struct HighlightData
{
  int lineNumber;
  int position;
  int length;
  Highlights color;
};

struct Editor
{
  std::vector<std::string> lines = {""};
  int x = 0, y = 0, maxY = 0, maxX = 0, rowOffset = 0, colOffset = 0;

  int snapX = 0;

  bool isChord = false;
  std::string chord = "";

  std::string message = "";

  bool inCmdMode = true;

  std::string fileName = "";

  bool isCFile = false;

  bool unSavedChanges = false;

  HighlightData findHighlight;
  bool isFindHighlight = false;
};

const std::string KEYWORDS[] = {
    "auto",
    "bool",
    "break",
    "case",
    "char",
    "class",
    "const",
    "continue",
    "default",
    "do",
    "double",
    "else",
    "enum",
    "extern",
    "float",
    "for",
    "goto",
    "if",
    "inline",
    "int",
    "long",
    "namespace",
    "private",
    "public",
    "register",
    "restrict",
    "return",
    "short",
    "signed",
    "sizeof",
    "static",
    "struct",
    "switch",
    "typedef",
    "union",
    "unsigned",
    "void",
    "volatile",
    "while"};

void saveToFile(Editor &e)
{
  std::ofstream file(e.fileName);

  for (int i = 0; i < e.lines.size(); i++)
  {
    file << e.lines[i];
    if (i < e.lines.size() - 1)
      file << std::endl;
  }

  file.close();

  e.unSavedChanges = false;
}

void refreshScreen(Editor &e)
{
  int maxLineNumberLength = std::to_string(e.lines.size()).size();

  getmaxyx(stdscr, e.maxY, e.maxX);
  e.maxY--;

  std::string lineNumberString = "";

  std::vector<HighlightData> highlights;

  if (!e.isChord)
  {
    if (e.isCFile)
    {
      bool inMultilineComment = false;

      int bracketLevel = 0;

      for (int i = 0; i < std::min(e.maxY + e.rowOffset, (int)e.lines.size()); i++)
      {
        std::string lineWithoutStrings = e.lines[i];

        int stringStart = lineWithoutStrings.find("\"");
        while (stringStart != std::string::npos)
        {
          int stringEnd = lineWithoutStrings.find("\"", stringStart + 1);

          if (!inMultilineComment)
            highlights.push_back({i, stringStart, stringEnd - stringStart + 1, STRING});

          if (stringEnd == std::string::npos)
            break;

          for (int j = stringStart; j < stringEnd; j++)
            lineWithoutStrings[j] = ' ';

          stringStart = lineWithoutStrings.find("\"", stringEnd + 1);
        }

        if (inMultilineComment)
        {
          int multilineCommentEnd = e.lines[i].find("*/");

          if (multilineCommentEnd != std::string::npos)
          {
            inMultilineComment = false;
            highlights.push_back({i, 0, multilineCommentEnd + 2, COMMENT});
          }
          else
          {
            highlights.push_back({i, 0, (int)e.lines[i].size(), COMMENT});
          }
        }
        else // Not in multiline comment
        {
          int multilineCommentStart = lineWithoutStrings.find("/*");

          if (multilineCommentStart != std::string::npos)
          {
            inMultilineComment = true;
            highlights.push_back({i, multilineCommentStart, (int)e.lines[i].size() - multilineCommentStart, COMMENT});
          }

          if (inMultilineComment)
            continue;

          int commentStart = lineWithoutStrings.find("//");

          if (commentStart != std::string::npos)
          {
            highlights.push_back({i, commentStart, (int)e.lines[i].size() - commentStart, COMMENT});

            lineWithoutStrings = lineWithoutStrings.substr(0, commentStart);
          }

          if (e.lines[i][0] == '#')
          {
            highlights.push_back({i, 0, (int)e.lines[i].size(), DIRECTIVE});

            if (e.lines[i].substr(0, 8) == "#include")
              highlights.push_back({i, 8, (int)e.lines[i].size() - 8, STRING});
            else if (e.lines[i].substr(0, 7) != "#define")
            {
              int lineStart = lineWithoutStrings.find_first_not_of(" \t");

              if (lineStart != std::string::npos)
              {
                int directiveArgsStart = lineWithoutStrings.find_first_of(" \t", lineStart);

                if (directiveArgsStart != std::string::npos)
                {
                  int directiveArgsEnd = lineWithoutStrings.find_first_of(" \t", directiveArgsStart + 1);

                  if (directiveArgsEnd == std::string::npos)
                    directiveArgsEnd = lineWithoutStrings.size();

                  highlights.push_back({i, directiveArgsStart + 1, directiveArgsEnd - directiveArgsStart - 1, KEYWORD});
                }
              }
            }

            lineWithoutStrings = "";
          }
          else
          {
            int angleBracketStart = lineWithoutStrings.find("<");

            while (angleBracketStart != std::string::npos)
            {
              int angleBracketEnd = lineWithoutStrings.find(">", angleBracketStart + 1);

              if (angleBracketEnd == std::string::npos)
                break;

              highlights.push_back({i, angleBracketStart + 1, angleBracketEnd - angleBracketStart - 1, KEYWORD});

              angleBracketStart = lineWithoutStrings.find("<", angleBracketEnd + 1);
            }
          }

          for (int j = 0; j < sizeof(KEYWORDS) / sizeof(KEYWORDS[0]); j++)
          {
            int keywordStart = lineWithoutStrings.find(KEYWORDS[j]);

            while (keywordStart != std::string::npos)
            {
              if (keywordStart == 0 || !isalnum(lineWithoutStrings[keywordStart - 1]))
              {
                if (keywordStart + KEYWORDS[j].size() == lineWithoutStrings.size() || !isalnum(lineWithoutStrings[keywordStart + KEYWORDS[j].size()]))
                {
                  highlights.push_back({i, keywordStart, (int)KEYWORDS[j].size(), KEYWORD});
                }
              }

              keywordStart = lineWithoutStrings.find(KEYWORDS[j], keywordStart + 1);
            }
          }

          int numberStart = lineWithoutStrings.find_first_of("0123456789");

          while (numberStart != std::string::npos)
          {
            int numberEnd = lineWithoutStrings.find_first_not_of("0123456789", numberStart);

            bool isNumber = true;

            if (numberStart != 0 && isalnum(lineWithoutStrings[numberStart - 1]))
              isNumber = false;

            if (numberEnd != std::string::npos && isalnum(lineWithoutStrings[numberEnd]))
              isNumber = false;

            if (isNumber)
              highlights.push_back({i, numberStart, numberEnd - numberStart, NUMBER});

            numberStart = lineWithoutStrings.find_first_of("0123456789", numberEnd);
          }

          for (int j = 0; j < lineWithoutStrings.size(); j++)
          {
            switch (lineWithoutStrings[j])
            {
            case '(':
            case '[':
            case '{':
              bracketLevel++;
              highlights.push_back({i, j, 1, (Highlights)BRACKET_HIGHLIGHTS[bracketLevel % 3]});
              break;
            case ')':
            case ']':
            case '}':
              highlights.push_back({i, j, 1, (Highlights)BRACKET_HIGHLIGHTS[bracketLevel % 3]});
              bracketLevel--;
              break;
            }
          }
        }
      }
    }

    clear();

    for (int i = 0; i < e.maxY; i++)
    {
      if (i + e.rowOffset < e.lines.size())
        lineNumberString += std::to_string(i + e.rowOffset + 1);
      else
        for (int j = 0; j < maxLineNumberLength; j++)
          lineNumberString += " ";

      lineNumberString += "\n";
    }

    attron(LINE_NUMBER);
    mvaddstr(0, 0, lineNumberString.c_str());
    attroff(LINE_NUMBER);

    for (int i = e.rowOffset; i < std::min(e.maxY + e.rowOffset, (int)e.lines.size()); i++)
    {
      std::string cutLine = e.lines[i].substr(std::min((int)e.lines[i].size(), e.colOffset), e.maxX - maxLineNumberLength - 1);

      int offseti = i - e.rowOffset;

      mvaddstr(offseti, maxLineNumberLength + 1, cutLine.c_str());
    }

    if (e.isCFile)
    {
      for (const HighlightData &highlight : highlights)
      {
        attron(highlight.color);
        mvaddstr(highlight.lineNumber - e.rowOffset,
                 maxLineNumberLength + 1 + highlight.position,
                 cut(e.lines[highlight.lineNumber].substr(highlight.position, highlight.length), highlight.position));
        attroff(highlight.color);
      }
    }

    if (e.isFindHighlight)
    {
      attron(e.findHighlight.color);
      mvaddstr(e.findHighlight.lineNumber - e.rowOffset,
               maxLineNumberLength + 1 + e.findHighlight.position,
               cut(e.lines[e.findHighlight.lineNumber].substr(e.findHighlight.position, e.findHighlight.length), e.findHighlight.position));
      attroff(e.findHighlight.color);

      e.isFindHighlight = false;
    }

    if (e.message.size() == 0)
    {
      e.message = e.fileName.substr(e.fileName.find_last_of("/") + 1, e.fileName.size() - e.fileName.find_last_of("/") - 1) + " - " + std::to_string(e.lines.size()) + " lines";

      if (e.unSavedChanges)
        e.message += " (modified)";
    }

    for (int i = 0; i < e.maxX; i++)
      mvaddstr(e.maxY, i, " ");
    attron(MESSAGE);
    mvaddstr(e.maxY, 0, e.message.c_str());
    attroff(MESSAGE);

    move(e.y - e.rowOffset, e.x + maxLineNumberLength + 1);
  }
  else
  {
    e.message = "";

    for (int i = 0; i < e.maxX; i++)
      mvaddstr(e.maxY, i, " ");

    attron(MESSAGE);
    mvaddstr(e.maxY, 0, e.chord.c_str());
    attroff(MESSAGE);
  }

  refresh();
}

int main(int argc, char **argv)
{
  Editor e;

  if (argc > 1)
  {
    std::ifstream file(argv[1]);

    e.fileName = argv[1];

    std::string fileExtension = e.fileName.substr(e.fileName.find_last_of(".") + 1, e.fileName.size() - e.fileName.find_last_of(".") - 1);

    if (fileExtension == "c" || fileExtension == "cpp" || fileExtension == "h" || fileExtension == "hpp")
      e.isCFile = true;

    if (file.is_open())
    {
      e.lines.clear();
      std::string line;
      while (std::getline(file, line))
      {
        e.lines.push_back(line);
      }

      file.close();
    }

    if (e.lines.size() == 0)
      e.lines.push_back("");
  }
  else
  {
    std::cerr << "No file specified" << std::endl;
    return 1;
  }

  set_escdelay(0);
  initscr();
  keypad(stdscr, TRUE);
  noecho();
  raw();
  start_color();
  use_default_colors();

  init_pair(WHITE, COLOR_WHITE, DEFAULT_BLACK);
  init_pair(CYAN, COLOR_CYAN, DEFAULT_BLACK);
  init_pair(RED, COLOR_RED, DEFAULT_BLACK);
  init_pair(GREEN, COLOR_GREEN, DEFAULT_BLACK);
  init_pair(YELLOW, COLOR_YELLOW, DEFAULT_BLACK);
  init_pair(BLUE, COLOR_BLUE, DEFAULT_BLACK);
  init_pair(MAGENTA, COLOR_MAGENTA, DEFAULT_BLACK);
  init_pair(CYAN_BACK, DEFAULT_BLACK, COLOR_CYAN);

  refreshScreen(e);

  while (true)
  {
    bool shouldRefresh = true;

    int ch = getch();
    int ch2;

    if (e.isChord)
    {
      if (ch == KEY_ENTER || ch == '\n')
      {
        e.chord = e.chord.substr(1, e.chord.size() - 1);

        if (e.chord == "q")
        {
          endwin();
          return 0;
        }
        else if (e.chord == "sq" || e.chord == "wq")
        {
          saveToFile(e);
          endwin();
          return 0;
        }
        else if (e.chord == "s" || e.chord == "w")
        {
          saveToFile(e);
          e.isChord = false;
        }
        else if (e.chord == "i")
        {
          e.inCmdMode = false;
          e.message = "INSERT - PRESS ESC TO EXIT";
        }
        else if (e.chord.substr(0, 2) == "l ")
        {
          std::string lineNumberString = e.chord.substr(2);

          if (lineNumberString == "e")
            lineNumberString = std::to_string(e.lines.size());
          else if (lineNumberString.size() == 0 || lineNumberString.find_first_not_of(" 0123456789") != std::string::npos || lineNumberString == "0")
            lineNumberString = "1";

          int lineNumber = std::min(std::stoi(lineNumberString), (int)e.lines.size());

          if (lineNumber > 0)
          {
            e.y = lineNumber - 1;
            e.x = 0;
            e.snapX = e.x;

            if (e.y < e.rowOffset || e.y >= e.rowOffset + getmaxy(stdscr))
              e.rowOffset = std::max(0, e.y - getmaxy(stdscr) / 2);
          }
        }
        else if (e.chord.substr(0, 2) == "f ")
        {
          std::string targetString = e.chord.substr(2, e.chord.size() - 2);

          if (targetString.size() != 0)
          {
            int lineNumber = e.y + 1;
            int positionx = 0;
            bool found = false;

            for (int i = 0; i < e.lines.size(); i++)
            {
              int offsetI = (i + e.y) % e.lines.size();

              if (e.lines[offsetI].find(targetString) != std::string::npos)
              {
                lineNumber = offsetI + 1;
                positionx = e.lines[offsetI].find(targetString);
                found = true;
                break;
              }
            }

            if (found)
            {
              e.y = lineNumber - 1;
              e.x = positionx;
              e.snapX = e.x;

              if (e.y < e.rowOffset || e.y > e.rowOffset + getmaxy(stdscr))
                e.rowOffset = std::max(0, e.y - getmaxy(stdscr) / 2);

              e.isFindHighlight = true;
              e.findHighlight = {e.y, e.x, (int)targetString.size(), FIND};
            }
            else
            {
              e.message = "NOT FOUND";
            }
          }
        }
        else if (e.chord.substr(0, 4) == "swp ")
        {
          saveToFile(e);

          std::string newFileName = e.chord.substr(4, e.chord.size() - 4);

          if (newFileName.size() != 0)
          {
            newFileName = newFileName.substr(0, newFileName.find_first_of(" "));

            std::ifstream file(newFileName);

            if (file.is_open())
            {
              e.fileName = newFileName;

              e.lines.clear();
              std::string line;
              while (std::getline(file, line))
              {
                e.lines.push_back(line);
              }
              if (e.lines.size() == 0)
                e.lines.push_back("");

              file.close();

              e.y = 0;
              e.x = 0;
              e.snapX = e.x;
              e.rowOffset = 0;
              e.colOffset = 0;
            }
            else
            {
              e.message = "FILE NOT FOUND - USE :cswp TO CREATE NEW FILE";
            }
          }
        }
        else if (e.chord.substr(0, 2) == "c ")
        {
          std::string newFileName = e.chord.substr(2, e.chord.size() - 2);

          if (newFileName.size() != 0)
          {
            std::ofstream file(newFileName);
            file.close();
          }
          else
          {
            e.message = "NO FILE NAME";
          }
        }
        else if (e.chord.substr(0, 5) == "cswp ")
        {
          std::string newFileName = e.chord.substr(5, e.chord.size() - 5);

          if (newFileName.size() != 0)
          {
            std::ofstream ofile(newFileName);
            ofile.close();

            e.fileName = newFileName.substr(0, newFileName.find_first_of(" "));

            std::ifstream file(e.fileName);

            e.lines.clear();
            std::string line;
            while (std::getline(file, line))
            {
              e.lines.push_back(line);
            }
            if (e.lines.size() == 0)
              e.lines.push_back("");

            file.close();

            e.y = 0;
            e.x = 0;
            e.snapX = e.x;
            e.rowOffset = 0;
            e.colOffset = 0;
          }
          else
          {
            e.message = "NO FILE NAME";
          }
        }
        else
        {
          e.message = "UNKNOWN COMMAND";
        }

        e.chord = "";
        e.isChord = false;
      }
      else if (ch == KEY_BACKSPACE || ch == 127)
      {
        if (e.chord.size() > 1)
          e.chord = e.chord.substr(0, e.chord.size() - 1);
        else
        {
          e.chord = "";
          e.isChord = false;
        }
      }
      else if (ch != ERR && isprint(ch))
      {
        e.chord += ch;
      }

      refreshScreen(e);
      continue;
    }

    switch (ch)
    {
    case KEY_UP:
      shouldRefresh = false;
      if (e.y > 0)
      {
        e.y--;
        if (e.y < e.rowOffset)
        {
          e.rowOffset--;
          shouldRefresh = true;
        }

        e.x = std::min(e.snapX, (int)e.lines[e.y].size());
      }
      else if (e.x > 0)
      {
        e.x = 0;
      }
      else
        e.snapX = e.x;
      break;

    case KEY_DOWN:
      shouldRefresh = false;
      if (e.y < e.lines.size() - 1)
      {
        e.maxY = getmaxy(stdscr) + e.rowOffset - 1;

        e.y++;
        if (e.y >= e.maxY)
        {
          e.rowOffset++;
          shouldRefresh = true;
        }

        e.x = std::min(e.snapX, (int)e.lines[e.y].size());
      }
      else if (e.x < e.lines[e.y].size())
      {
        e.x = e.lines[e.y].size();
      }
      else
        e.snapX = e.x;
      break;

    case KEY_LEFT:
      shouldRefresh = false;
      if (e.x > 0)
      {
        e.x--;
      }
      else if (e.y > 0)
      {
        e.y--;
        if (e.y < e.rowOffset)
        {
          e.rowOffset--;
          shouldRefresh = true;
        }
        e.x = e.lines[e.y].size();
      }
      e.snapX = e.x;
      break;

    case KEY_RIGHT:
      shouldRefresh = false;
      if (e.x < e.lines[e.y].size())
      {
        e.x++;
        e.maxX = getmaxx(stdscr);
      }
      else if (e.y < e.lines.size() - 1)
      {
        e.maxY = getmaxy(stdscr) + e.rowOffset - 1;

        e.y++;
        if (e.y >= e.maxY)
        {
          e.rowOffset++;
          shouldRefresh = true;
        }

        e.x = 0;
      }
      e.snapX = e.x;
      break;

    case 127:
    case '\b':
    case KEY_BACKSPACE:
      if (e.inCmdMode)
        break;

      if (e.x > 0)
      {
        e.lines[e.y].erase(e.x - 1, 1);
        e.x--;

        e.unSavedChanges = true;
      }
      else if (e.y > 0)
      {
        e.x = e.lines[e.y - 1].size();
        e.lines[e.y - 1] += e.lines[e.y];
        e.lines.erase(e.lines.begin() + e.y);
        e.y--;

        if (e.y < e.rowOffset)
          e.rowOffset--;

        e.unSavedChanges = true;
      }

      e.snapX = e.x;

      break;

    case '\n':
    case KEY_ENTER:
      if (e.inCmdMode)
        break;

      e.unSavedChanges = true;

      e.lines.insert(e.lines.begin() + e.y + 1, e.lines[e.y].substr(e.x));
      e.lines[e.y].erase(e.x);

      e.y++;
      if (e.y >= getmaxy(stdscr) + e.rowOffset - 1)
        e.rowOffset++;

      e.x = 0;
      e.snapX = e.x;
      break;

    case '\t':
      if (e.inCmdMode)
        break;

      e.unSavedChanges = true;

      e.lines[e.y].insert(e.x, 4, ' ');
      e.x += 4;
      e.snapX = e.x;
      break;

    case 27:
      e.inCmdMode = true;
      e.message = "";
      break;

    default:
      if (e.inCmdMode)
      {
        switch (ch)
        {
        case ':':
        case ';':
          e.isChord = true;
          e.chord = ch;
          break;

        case 'i':
          e.inCmdMode = false;
          e.message = "INSERT - PRESS ESC TO EXIT";
          break;

        case 'w':
          shouldRefresh = false;
          if (e.y > 0)
          {
            e.y--;
            if (e.y < e.rowOffset)
            {
              e.rowOffset--;
              shouldRefresh = true;
            }

            e.x = std::min(e.snapX, (int)e.lines[e.y].size());
          }
          else if (e.x > 0)
          {
            e.x = 0;
          }
          else
            e.snapX = e.x;
          break;

        case 's':
          shouldRefresh = false;
          if (e.y < e.lines.size() - 1)
          {
            e.maxY = getmaxy(stdscr) + e.rowOffset - 1;

            e.y++;
            if (e.y >= e.maxY)
            {
              e.rowOffset++;
              shouldRefresh = true;
            }

            e.x = std::min(e.snapX, (int)e.lines[e.y].size());
          }
          else if (e.x < e.lines[e.y].size())
          {
            e.x = e.lines[e.y].size();
          }
          else
            e.snapX = e.x;
          break;

        case 'a':
          e.x = 0;
          e.snapX = e.x;
          break;

        case 'd':
          e.x = e.lines[e.y].size();
          e.snapX = e.x;
          break;
        }

        break;
      }

      if (ch != ERR && isprint(ch))
      {
        e.lines[e.y].insert(e.x, 1, ch);
        e.x++;
        e.snapX = e.x;

        e.unSavedChanges = true;
      }
      break;
    }

    if (shouldRefresh)
    {
      refreshScreen(e);
    }
    else
    {
      int maxLineNumberLength = std::to_string(e.lines.size()).size();
      move(e.y - e.rowOffset, e.x + maxLineNumberLength + 1);
    }
  }

  endwin();
  return 0;
}