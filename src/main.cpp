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

void saveToFile(std::string fileName)
{
  std::ofstream file(fileName);

  for (int i = 0; i < lines.size(); i++)
  {
    file << lines[i];
    if (i < lines.size() - 1)
      file << std::endl;
  }

  file.close();

  unSavedChanges = false;
}

void refreshScreen()
{
  int maxLineNumberLength = std::to_string(lines.size()).size();

  getmaxyx(stdscr, maxY, maxX);
  maxY--;

  std::string lineNumberString = "";

  if (isChord)
    goto chord;

  clear();

  for (int i = 0; i < maxY; i++)
  {
    if (i + rowOffset < lines.size())
      lineNumberString += std::to_string(i + rowOffset + 1);
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

  for (int i = rowOffset; i < std::min(maxY + rowOffset, (int)lines.size()); i++)
  {
    std::string cutLine = lines[i].substr(std::min((int)lines[i].size(), colOffset), maxX - maxLineNumberLength - 1);

    int offseti = i - rowOffset;

    mvaddstr(offseti, maxLineNumberLength + 1, cutLine.c_str());
  }

  if (message.size() == 0)
  {
    // message = fileName + " - " + std::to_string(lines.size()) + " lines";
    message = fileName.substr(fileName.find_last_of("/") + 1, fileName.size() - fileName.find_last_of("/") - 1) + " - " + std::to_string(lines.size()) + " lines";

    if (unSavedChanges)
      message += " (modified)";
  }

  for (int i = 0; i < maxX; i++)
    mvaddstr(maxY, i, " ");
  attron(COLOR_PAIR(GREEN));
  attron(A_BOLD);
  mvaddstr(maxY, 0, message.c_str());
  attroff(COLOR_PAIR(GREEN));
  attroff(A_BOLD);

  move(y - rowOffset, x + maxLineNumberLength + 1);

  if (isChord)
  {
  chord:
    message = "";

    for (int i = 0; i < maxX; i++)
      mvaddstr(maxY, i, " ");

    attron(COLOR_PAIR(GREEN));
    attron(A_BOLD);
    mvaddstr(maxY, 0, chord.c_str());
    attroff(COLOR_PAIR(GREEN));
    attroff(A_BOLD);
  }

  refresh();
}

int main(int argc, char **argv)
{
  if (argc > 1)
  {
    std::ifstream file(argv[1]);

    fileName = argv[1];

    if (file.is_open())
    {
      lines.clear();
      std::string line;
      while (std::getline(file, line))
      {
        lines.push_back(line);
      }

      file.close();
    }

    if (lines.size() == 0)
      lines.push_back("");
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

  refreshScreen();

  while (true)
  {
    bool shouldRefresh = true;

    int ch = getch();

    if (isChord)
    {
      if (ch == KEY_ENTER || ch == '\n')
      {
        chord[0] = ':';
        if (chord == ":q")
        {
          endwin();
          return 0;
        }
        else if (chord == ":sq" || chord == ":wq")
        {
          saveToFile(fileName);
          endwin();
          return 0;
        }
        else if (chord == ":s" || chord == ":w")
        {
          saveToFile(fileName);
          isChord = false;
        }
        else if (chord == ":i")
        {
          inCmdMode = false;
          message = "INSERT - PRESS ESC TO EXIT";
        }
        else if (chord.substr(0, 3) == ":l ")
        {
          std::string lineNumberString = chord.substr(3, chord.size() - 3);

          if (lineNumberString.size() == 0 || lineNumberString.find_first_not_of(" 0123456789") != std::string::npos)
            lineNumberString = "1";

          int lineNumber = std::min(std::stoi(lineNumberString), (int)lines.size());

          if (lineNumber > 0)
          {
            y = lineNumber - 1;
            x = 0;
            rowOffset = std::max(0, y - getmaxy(stdscr) / 2);
          }
        }
        else if (chord.substr(0, 3) == ":f ")
        {
          std::string targetString = chord.substr(3, chord.size() - 3);

          if (targetString.size() != 0)
          {
            int lineNumber = y + 1;
            int positionx = 0;
            bool found = false;

            for (int i = 0; i < lines.size(); i++)
            {
              int offsetI = (i + y) % lines.size();

              if (lines[offsetI].find(targetString) != std::string::npos)
              {
                lineNumber = offsetI + 1;
                positionx = lines[offsetI].find(targetString);
                found = true;
                break;
              }
            }

            if (found)
            {
              y = lineNumber - 1;
              x = positionx;
              rowOffset = std::max(0, y - getmaxy(stdscr) / 2);
            }
            else
            {
              message = "NOT FOUND";
            }
          }
        }
        else if (chord.substr(0, 5) == ":swp ")
        {
          saveToFile(fileName);

          std::string newFileName = chord.substr(5, chord.size() - 5);

          if (newFileName.size() != 0)
          {
            newFileName = newFileName.substr(0, newFileName.find_first_of(" "));

            std::ifstream file(newFileName);

            if (file.is_open())
            {
              fileName = newFileName;

              lines.clear();
              std::string line;
              while (std::getline(file, line))
              {
                lines.push_back(line);
              }
              if (lines.size() == 0)
                lines.push_back("");

              file.close();

              y = 0;
              x = 0;
              rowOffset = 0;
              colOffset = 0;
            }
            else
            {
              message = "FILE NOT FOUND - USE :cswp TO CREATE NEW FILE";
            }
          }
        }
        else if (chord.substr(0, 3) == ":c ")
        {
          std::string newFileName = chord.substr(3, chord.size() - 3);

          if (newFileName.size() != 0)
          {
            std::ofstream file(newFileName);
            file.close();
          }
          else
          {
            message = "NO FILE NAME";
          }
        }
        else if (chord.substr(0, 6) == ":cswp ")
        {
          std::string newFileName = chord.substr(6, chord.size() - 6);

          if (newFileName.size() != 0)
          {
            std::ofstream ofile(newFileName);
            ofile.close();

            fileName = newFileName.substr(0, newFileName.find_first_of(" "));

            std::ifstream file(fileName);

            lines.clear();
            std::string line;
            while (std::getline(file, line))
            {
              lines.push_back(line);
            }
            if (lines.size() == 0)
              lines.push_back("");

            file.close();

            y = 0;
            x = 0;
            rowOffset = 0;
            colOffset = 0;
          }
          else
          {
            message = "NO FILE NAME";
          }
        }
        else
        {
          message = "UNKNOWN COMMAND";
        }

        chord = "";
        isChord = false;
      }

      if (ch != ERR && isprint(ch))
        chord += ch;
      else if (ch == KEY_BACKSPACE || ch == 127)
      {
        if (chord.size() > 1)
          chord = chord.substr(0, chord.size() - 1);
        else
        {
          chord = "";
          isChord = false;
        }
      }

      refreshScreen();
      continue;
    }

    switch (ch)
    {
    case KEY_UP:
      shouldRefresh = false;
      if (y > 0)
      {
        y--;
        if (y < rowOffset)
        {
          rowOffset--;
          shouldRefresh = true;
        }

        x = std::min(x, (int)lines[y].size());
      }
      break;

    case KEY_DOWN:
      shouldRefresh = false;
      if (y < lines.size() - 1)
      {
        maxY = getmaxy(stdscr) + rowOffset - 1;

        y++;
        if (y >= maxY)
        {
          rowOffset++;
          shouldRefresh = true;
        }

        x = std::min(x, (int)lines[y].size());
      }
      break;

    case KEY_LEFT:
      shouldRefresh = false;
      if (x > 0)
      {
        x--;
      }
      else if (y > 0)
      {
        y--;
        if (y < rowOffset)
        {
          rowOffset--;
          shouldRefresh = true;
        }
        x = lines[y].size();
      }
      break;

    case KEY_RIGHT:
      shouldRefresh = false;
      if (x < lines[y].size())
      {
        x++;
        maxX = getmaxx(stdscr);
      }
      else if (y < lines.size() - 1)
      {
        maxY = getmaxy(stdscr) + rowOffset - 1;

        y++;
        if (y >= maxY)
        {
          rowOffset++;
          shouldRefresh = true;
        }

        x = 0;
      }
      break;

    case 127:
    case '\b':
    case KEY_BACKSPACE:
      if (inCmdMode)
        break;

      if (x > 0)
      {
        lines[y].erase(x - 1, 1);
        x--;

        unSavedChanges = true;
      }
      else if (y > 0)
      {
        x = lines[y - 1].size();
        lines[y - 1] += lines[y];
        lines.erase(lines.begin() + y);
        y--;

        if (y < rowOffset)
          rowOffset--;

        unSavedChanges = true;
      }

      break;

    case '\n':
    case KEY_ENTER:
      if (inCmdMode)
        break;

      unSavedChanges = true;

      lines.insert(lines.begin() + y + 1, lines[y].substr(x));
      lines[y].erase(x);

      y++;
      if (y >= getmaxy(stdscr) + rowOffset - 1)
        rowOffset++;

      x = 0;
      break;

    case '\t':
      if (inCmdMode)
        break;

      unSavedChanges = true;

      lines[y].insert(x, 4, ' ');
      x += 4;
      break;

    case 27:
      inCmdMode = true;
      message = "";
      break;

    case ':':
    case ';':
      if (inCmdMode)
      {
        isChord = true;
        chord = ch;
        break;
      }

    case 'i':
      if (inCmdMode)
      {
        inCmdMode = false;
        message = "INSERT - PRESS ESC TO EXIT";
        break;
      }

    default:
      if (inCmdMode)
        break;

      if (ch != ERR && isprint(ch))
      {
        lines[y].insert(x, 1, ch);
        x++;

        unSavedChanges = true;
      }
      break;
    }

    if (shouldRefresh)
    {
      refreshScreen();
    }
    else
    {
      int maxLineNumberLength = std::to_string(lines.size()).size();
      move(y - rowOffset, x + maxLineNumberLength + 1);
    }
  }

  endwin();
  return 0;
}