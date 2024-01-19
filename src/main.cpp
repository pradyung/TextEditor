#include <ncurses.h>
#include <cctype>
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>
#include <iostream>

enum Colors
{
  WHITE,
  CYAN,
  RED,
  GREEN,
  YELLOW,
  BLUE,
  MAGENTA
};

struct Editor
{
  std::vector<std::string> lines = {""};
  int x, y;
  int maxY, maxX;

  int rowOffset = 0;
  int colOffset = 0;

  bool isChord = false;
  std::string chord = "";

  std::string message = "";

  bool inCmdMode = true;

  std::string fileName = "";

  bool unSavedChanges = false;
};

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

  if (e.isChord)
    goto chord;

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

  attron(COLOR_PAIR(CYAN));
  attron(A_BOLD);
  mvaddstr(0, 0, lineNumberString.c_str());
  attroff(COLOR_PAIR(CYAN));
  attroff(A_BOLD);

  for (int i = e.rowOffset; i < std::min(e.maxY + e.rowOffset, (int)e.lines.size()); i++)
  {
    std::string cutLine = e.lines[i].substr(std::min((int)e.lines[i].size(), e.colOffset), e.maxX - maxLineNumberLength - 1);

    int offseti = i - e.rowOffset;

    mvaddstr(offseti, maxLineNumberLength + 1, cutLine.c_str());
  }

  if (e.message.size() == 0)
  {
    e.message = e.fileName.substr(e.fileName.find_last_of("/") + 1, e.fileName.size() - e.fileName.find_last_of("/") - 1) + " - " + std::to_string(e.lines.size()) + " lines";

    if (e.unSavedChanges)
      e.message += " (modified)";
  }

  for (int i = 0; i < e.maxX; i++)
    mvaddstr(e.maxY, i, " ");
  attron(COLOR_PAIR(GREEN));
  attron(A_BOLD);
  mvaddstr(e.maxY, 0, e.message.c_str());
  attroff(COLOR_PAIR(GREEN));
  attroff(A_BOLD);

  move(e.y - e.rowOffset, e.x + maxLineNumberLength + 1);

  if (e.isChord)
  {
  chord:
    e.message = "";

    for (int i = 0; i < e.maxX; i++)
      mvaddstr(e.maxY, i, " ");

    attron(COLOR_PAIR(GREEN));
    attron(A_BOLD);
    mvaddstr(e.maxY, 0, e.chord.c_str());
    attroff(COLOR_PAIR(GREEN));
    attroff(A_BOLD);
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

  set_escdelay(25);
  initscr();
  keypad(stdscr, TRUE);
  noecho();
  raw();
  start_color();

  init_pair(WHITE, COLOR_WHITE, COLOR_BLACK);
  init_pair(CYAN, COLOR_CYAN, COLOR_BLACK);
  init_pair(RED, COLOR_RED, COLOR_BLACK);
  init_pair(GREEN, COLOR_GREEN, COLOR_BLACK);
  init_pair(YELLOW, COLOR_YELLOW, COLOR_BLACK);
  init_pair(BLUE, COLOR_BLUE, COLOR_BLACK);
  init_pair(MAGENTA, COLOR_MAGENTA, COLOR_BLACK);

  refreshScreen(e);

  while (true)
  {
    bool shouldRefresh = true;

    int ch = getch();

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
          std::string lineNumberString = e.chord.substr(3, e.chord.size() - 3);

          if (lineNumberString.size() == 0 || lineNumberString.find_first_not_of(" 0123456789") != std::string::npos)
            lineNumberString = "1";

          int lineNumber = std::min(std::stoi(lineNumberString), (int)e.lines.size());

          if (lineNumber > 0)
          {
            e.y = lineNumber - 1;
            e.x = 0;
            e.rowOffset = std::max(0, e.y - getmaxy(stdscr) / 2);
          }
        }
        else if (e.chord.substr(0, 2) == "f ")
        {
          std::string targetString = e.chord.substr(3, e.chord.size() - 3);

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
              e.rowOffset = std::max(0, e.y - getmaxy(stdscr) / 2);
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

          std::string newFileName = e.chord.substr(5, e.chord.size() - 5);

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
          std::string newFileName = e.chord.substr(3, e.chord.size() - 3);

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
          std::string newFileName = e.chord.substr(6, e.chord.size() - 6);

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

      if (ch != ERR && isprint(ch))
        e.chord += ch;
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

        e.x = std::min(e.x, (int)e.lines[e.y].size());
      }
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

        e.x = std::min(e.x, (int)e.lines[e.y].size());
      }
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
      break;

    case '\t':
      if (e.inCmdMode)
        break;

      e.unSavedChanges = true;

      e.lines[e.y].insert(e.x, 4, ' ');
      e.x += 4;
      break;

    case 27:
      e.inCmdMode = true;
      e.message = "";
      break;

    case ':':
    case ';':
      if (e.inCmdMode)
      {
        e.isChord = true;
        e.chord = ch;
        break;
      }

    case 'i':
      if (e.inCmdMode)
      {
        e.inCmdMode = false;
        e.message = "INSERT - PRESS ESC TO EXIT";
        break;
      }

    default:
      if (e.inCmdMode)
        break;

      if (ch != ERR && isprint(ch))
      {
        e.lines[e.y].insert(e.x, 1, ch);
        e.x++;

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