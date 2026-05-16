#include <SFML/Graphics.hpp>
#include <SFML/Window/Clipboard.hpp>
#include <iostream>
#include <string>
#include <fstream>
#include <windows.h>
#include <optional>
#include <map>
#include <vector>
#include <algorithm>


std::string executeOSCommand(std::string cmd) {
    HANDLE hRead, hWrite;
    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };

    if (!CreatePipe(&hRead, &hWrite, &sa, 0)) return "Error: Pipe creation failed.\n";

    STARTUPINFOA si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdOutput = hWrite;
    si.hStdError = hWrite;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi));

    cmd = "cmd.exe /c " + cmd;
    if (!CreateProcessA(NULL, (LPSTR)cmd.c_str(), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        CloseHandle(hRead); CloseHandle(hWrite);
        return "OS Error: Could not create process.\n";
    }

    CloseHandle(hWrite);

    char buffer[256];
    DWORD bytesRead;
    std::string result = "";
    while (ReadFile(hRead, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead != 0) {
        buffer[bytesRead] = '\0';
        result += buffer;
    }

    CloseHandle(hRead);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return result;
}

bool isButtonClicked(sf::RectangleShape& button, sf::Vector2f mousePos) {
    return button.getGlobalBounds().contains(mousePos);
}

struct TerminalLine {
    std::string text;
    sf::Color color;
};

// ==========================================
// MAIN IDE APPLICATION
// ==========================================
int main() {
    auto desktop = sf::VideoMode::getDesktopMode();
    sf::RenderWindow window(desktop, "CodeShell IDE - Pro Dashboard", sf::Style::Default);
    window.setPosition(sf::Vector2i(0, 0));

    sf::Font font;
    if (!font.openFromFile("C:\\Windows\\Fonts\\consola.ttf")) {
        if (!font.openFromFile("C:\\Windows\\Fonts\\arial.ttf")) return -1;
    }

 
    std::map<std::string, std::string> files;
    files["main.cpp"] = "// Premium C++ Code Layout\n// Try Ctrl+A to select all text!\n// Scroll, Drag splitters, or click inside terminal to clear.\n\n#include <iostream>\nusing namespace std;\n\nint main() {\n    cout << \"CodeShell Premium Build Active!\" << endl;\n    return 0;\n}";
    files["script.py"] = "# python\n# Python Subsystem\nprint(\"Dynamic Dashboard Working Fine.\")";

    std::string currentFile = "main.cpp";
    int cursorIndex = files[currentFile].size();

    int editorScrollY = 0;
    int termScrollY = 0;

    float W = static_cast<float>(window.getSize().x);
    float H = static_cast<float>(window.getSize().y);
    float splitVertical = W * 0.30f; // Beautiful 30-70 ratio split
    float splitHorizontal = H * 0.45f;

    bool isDraggingVert = false;
    bool isDraggingHoriz = false;
    bool isModalOpen = false;
    bool isTextSelectedAll = false; // CTR+A Selection Toggle Flag

    std::string modalInput = "";

    std::vector<TerminalLine> terminalBuffer;
    terminalBuffer.push_back({"user@codeshell:~/project$ System Initialized smoothly.", sf::Color(100, 255, 100)});

    sf::RectangleShape cursor(sf::Vector2f(2.f, 20.f));
    cursor.setFillColor(sf::Color(255, 200, 0)); // Sleek amber-gold cursor
    sf::Clock blinkClock;

   
    while (window.isOpen()) {
        W = static_cast<float>(window.getSize().x);
        H = static_cast<float>(window.getSize().y);

        // UI Dashboard Theme Elements (Colors polished to look highly modern)
        sf::RectangleShape toolbarBg(sf::Vector2f(W, 50.f)); toolbarBg.setFillColor(sf::Color(30, 30, 30));
        sf::RectangleShape sidebarBg(sf::Vector2f(splitVertical, splitHorizontal - 50.f)); sidebarBg.setPosition(sf::Vector2f(0.f, 50.f)); sidebarBg.setFillColor(sf::Color(24, 24, 24));
        sf::RectangleShape terminalBg(sf::Vector2f(splitVertical, H - splitHorizontal)); terminalBg.setPosition(sf::Vector2f(0.f, splitHorizontal)); terminalBg.setFillColor(sf::Color(15, 15, 15));
        sf::RectangleShape gutterBg(sf::Vector2f(45.f, H - 50.f)); gutterBg.setPosition(sf::Vector2f(splitVertical, 50.f)); gutterBg.setFillColor(sf::Color(28, 28, 28));
        sf::RectangleShape editorBg(sf::Vector2f(W - splitVertical - 45.f, H - 50.f)); editorBg.setPosition(sf::Vector2f(splitVertical + 45.f, 50.f)); editorBg.setFillColor(sf::Color(20, 20, 20));

        // Draggable Lines
        sf::RectangleShape vertSplitter(sf::Vector2f(6.f, H - 50.f)); vertSplitter.setPosition(sf::Vector2f(splitVertical - 3.f, 50.f)); vertSplitter.setFillColor(isDraggingVert ? sf::Color(0, 122, 204) : sf::Color(45, 45, 45));
        sf::RectangleShape horizSplitter(sf::Vector2f(splitVertical, 6.f)); horizSplitter.setPosition(sf::Vector2f(0.f, splitHorizontal - 3.f)); horizSplitter.setFillColor(isDraggingHoriz ? sf::Color(0, 122, 204) : sf::Color(45, 45, 45));

        // Interaction Macro Buttons
        sf::RectangleShape btnRun(sf::Vector2f(90.f, 32.f)); btnRun.setPosition(sf::Vector2f(splitVertical + 60.f, 9.f)); btnRun.setFillColor(sf::Color(46, 117, 89));
        sf::Text textRun(font, "RUN", 15); textRun.setPosition(sf::Vector2f(splitVertical + 92.f, 15.f)); textRun.setFillColor(sf::Color::White);

        sf::RectangleShape btnNew(sf::Vector2f(110.f, 32.f)); btnNew.setPosition(sf::Vector2f(10.f, 9.f)); btnNew.setFillColor(sf::Color(30, 90, 150));
        sf::Text textNew(font, "+ NEW FILE", 14); textNew.setPosition(sf::Vector2f(22.f, 15.f)); textNew.setFillColor(sf::Color::White);

        sf::RectangleShape btnClear(sf::Vector2f(110.f, 32.f)); btnClear.setPosition(sf::Vector2f(130.f, 9.f)); btnClear.setFillColor(sf::Color(160, 50, 50));
        sf::Text textClear(font, "CLEAR ALL", 14); textClear.setPosition(sf::Vector2f(145.f, 15.f)); textClear.setFillColor(sf::Color::White);

        // NEW: Inline Terminal Mini Clear Button [X]
        sf::RectangleShape btnTermClear(sf::Vector2f(24.f, 20.f));
        btnTermClear.setPosition(sf::Vector2f(splitVertical - 35.f, splitHorizontal + 8.f));
        btnTermClear.setFillColor(sf::Color(50, 50, 50));
        sf::Text textTermClear(font, "X", 13);
        textTermClear.setPosition(sf::Vector2f(splitVertical - 27.f, splitHorizontal + 10.f));
        textTermClear.setFillColor(sf::Color(200, 100, 100));

        while (const std::optional<sf::Event> event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();

            // MOUSE MODULE
            if (const auto* mouseEvent = event->getIf<sf::Event::MouseButtonPressed>()) {
                sf::Vector2f mPos(static_cast<float>(mouseEvent->position.x), static_cast<float>(mouseEvent->position.y));
                isTextSelectedAll = false; // Any single click resets the Ctrl+A selection state

                if (mouseEvent->button == sf::Mouse::Button::Right) {
                    std::string clipboard = sf::Clipboard::getString().toAnsiString();
                    if (isModalOpen) modalInput += clipboard;
                    else if (mPos.x > splitVertical) {
                        files[currentFile].insert(cursorIndex, clipboard);
                        cursorIndex += clipboard.size();
                    }
                }

                if (mouseEvent->button == sf::Mouse::Button::Left) {
                    if (vertSplitter.getGlobalBounds().contains(mPos)) { isDraggingVert = true; continue; }
                    if (horizSplitter.getGlobalBounds().contains(mPos)) { isDraggingHoriz = true; continue; }

                    if (!isModalOpen) {
                        if (isButtonClicked(btnRun, mPos)) {
                            terminalBuffer.push_back({"user@codeshell:~/project$ Running " + currentFile + "...", sf::Color(100, 200, 255)});
                            bool isPython = (files[currentFile].find("# python") != std::string::npos);

                            std::ofstream file(isPython ? "temp.py" : "temp.cpp"); file << files[currentFile]; file.close();

                            if (isPython) {
                                terminalBuffer.push_back({executeOSCommand("python temp.py 2>&1"), sf::Color(240, 200, 80)});
                            } else {
                                std::string compOut = executeOSCommand("g++ temp.cpp -o temp.exe 2>&1");
                                if (compOut.empty()) terminalBuffer.push_back({executeOSCommand("temp.exe"), sf::Color(240, 200, 80)});
                                else terminalBuffer.push_back({"[COMPILER ERROR]\n" + compOut, sf::Color(255, 90, 90)});
                            }
                            termScrollY = 0;
                        }

                        if (isButtonClicked(btnNew, mPos)) { isModalOpen = true; modalInput = ""; }
                        if (isButtonClicked(btnClear, mPos)) { files[currentFile].clear(); cursorIndex = 0; }

                        // Action handling for inline terminal clear button
                        if (isButtonClicked(btnTermClear, mPos)) { terminalBuffer.clear(); termScrollY = 0; }

                        float fileY = 125.f;
                        for (auto const& [fileName, fileContent] : files) {
                            if (mPos.x < splitVertical && mPos.y >= fileY && mPos.y <= fileY + 25.f) {
                                currentFile = fileName;
                                cursorIndex = files[currentFile].size();
                                editorScrollY = 0;
                                break;
                            }
                            fileY += 25.f;
                        }

                        if (mPos.x > splitVertical && mPos.y > 50.f) {
                            int clickedLine = static_cast<int>((mPos.y - 50.f + editorScrollY) / 20.f);
                            int currentL = 0, charCount = 0;
                            for(char c : files[currentFile]) {
                                if (currentL == clickedLine) break;
                                if (c == '\n') currentL++;
                                charCount++;
                            }
                            cursorIndex = charCount;
                        }
                    }
                }
            }

            if (event->is<sf::Event::MouseButtonReleased>()) {
                isDraggingVert = false; isDraggingHoriz = false;
            }

            if (const auto* mouseMove = event->getIf<sf::Event::MouseMoved>()) {
                if (isDraggingVert) { splitVertical = std::clamp(static_cast<float>(mouseMove->position.x), 200.f, W - 300.f); }
                if (isDraggingHoriz) { splitHorizontal = std::clamp(static_cast<float>(mouseMove->position.y), 100.f, H - 200.f); }
            }

            if (const auto* scrollEvent = event->getIf<sf::Event::MouseWheelScrolled>()) {
                if (!isModalOpen) {
                    sf::Vector2i mPos = sf::Mouse::getPosition(window);
                    if (mPos.x > splitVertical) {
                        editorScrollY -= static_cast<int>(scrollEvent->delta) * 40; if (editorScrollY < 0) editorScrollY = 0;
                    } else if (mPos.y > splitHorizontal) {
                        termScrollY -= static_cast<int>(scrollEvent->delta) * 40; if (termScrollY < 0) termScrollY = 0;
                    }
                }
            }

            // KEYBOARD INPUT CONSOLE INTERFACES
            if (const auto* keyEvent = event->getIf<sf::Event::KeyPressed>()) {
                bool isCtrlPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RControl);

                if (isModalOpen) {
                    if (keyEvent->code == sf::Keyboard::Key::Enter && !modalInput.empty()) {
                        files[modalInput] = "// " + modalInput + "\n\n"; currentFile = modalInput; cursorIndex = files[currentFile].size(); isModalOpen = false;
                    }
                    else if (keyEvent->code == sf::Keyboard::Key::Escape) isModalOpen = false;
                } else {
                    // NEW: Ctrl + A Handling
                    if (keyEvent->code == sf::Keyboard::Key::A && isCtrlPressed) {
                        isTextSelectedAll = true;
                    }
                    // Paste execution block
                    if (keyEvent->code == sf::Keyboard::Key::V && isCtrlPressed) {
                        std::string clipboard = sf::Clipboard::getString().toAnsiString();
                        if (isTextSelectedAll) { files[currentFile] = clipboard; cursorIndex = clipboard.size(); isTextSelectedAll = false; }
                        else { files[currentFile].insert(cursorIndex, clipboard); cursorIndex += clipboard.size(); }
                    }
                }
            }

            if (const auto* textEvent = event->getIf<sf::Event::TextEntered>()) {
                bool isCtrlPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RControl);

                if (isModalOpen) {
                    if (textEvent->unicode == '\b' && !modalInput.empty()) modalInput.pop_back();
                    else if (textEvent->unicode >= 32 && textEvent->unicode < 127) modalInput += static_cast<char>(textEvent->unicode);
                } else {
                    if (!isCtrlPressed) {
                        std::string& text = files[currentFile];
                        // If typing while Ctrl+A is active, overwrite the file completely
                        if (isTextSelectedAll && textEvent->unicode != '\b') { text.clear(); cursorIndex = 0; isTextSelectedAll = false; }

                        if (textEvent->unicode == '\b') {
                            if (isTextSelectedAll) { text.clear(); cursorIndex = 0; isTextSelectedAll = false; }
                            else if (cursorIndex > 0) { text.erase(cursorIndex - 1, 1); cursorIndex--; }
                        }
                        else if (textEvent->unicode == '\r' || textEvent->unicode == '\n') {
                            text.insert(cursorIndex, 1, '\n'); cursorIndex++;
                        }
                        else if (textEvent->unicode < 128 && textEvent->unicode > 31) {
                            text.insert(cursorIndex, 1, static_cast<char>(textEvent->unicode)); cursorIndex++;
                        }
                    }
                }
            }
        }

        window.clear();

        window.draw(toolbarBg); window.draw(sidebarBg); window.draw(gutterBg); window.draw(editorBg); window.draw(terminalBg);

        // --- EDITOR BLOCK RENDERING VIEWPORTS ---
        sf::View editorView;
        editorView.setSize(sf::Vector2f(editorBg.getSize().x, editorBg.getSize().y));
        editorView.setCenter(sf::Vector2f(editorBg.getSize().x / 2.f, editorBg.getSize().y / 2.f));
        editorView.setViewport(sf::FloatRect(sf::Vector2f(editorBg.getPosition().x / W, editorBg.getPosition().y / H), sf::Vector2f(editorBg.getSize().x / W, editorBg.getSize().y / H)));
        window.setView(editorView);

        // High Visibility Code Text Structure
        sf::Text editorTextDisplay(font, files[currentFile], 18);
        // If text is selected via Ctrl+A, color it in cyan highlight hue
        editorTextDisplay.setFillColor(isTextSelectedAll ? sf::Color(50, 150, 200) : sf::Color(230, 230, 230));
        editorTextDisplay.setPosition(sf::Vector2f(10.f, 10.f - editorScrollY));
        window.draw(editorTextDisplay);

        sf::Vector2f cursorPos = editorTextDisplay.findCharacterPos(cursorIndex);
        cursor.setPosition(sf::Vector2f(cursorPos.x + 1.f, cursorPos.y + 4.f));
        if (blinkClock.getElapsedTime().asMilliseconds() % 1000 < 500 && !isModalOpen && !isTextSelectedAll) window.draw(cursor);

        // --- TERMINAL VIEW BLOCK ---
        sf::View termView;
        termView.setSize(sf::Vector2f(terminalBg.getSize().x, terminalBg.getSize().y));
        termView.setCenter(sf::Vector2f(terminalBg.getSize().x / 2.f, terminalBg.getSize().y / 2.f));
        termView.setViewport(sf::FloatRect(sf::Vector2f(terminalBg.getPosition().x / W, terminalBg.getPosition().y / H), sf::Vector2f(terminalBg.getSize().x / W, terminalBg.getSize().y / H)));
        window.setView(termView);

        sf::Text termHeader(font, "TERMINAL RUNTIME MANAGER\n-----------------------------------", 15);
        termHeader.setFillColor(sf::Color(130, 130, 130));
        termHeader.setPosition(sf::Vector2f(10.f, 10.f - termScrollY));
        window.draw(termHeader);

        float tY = 50.f - termScrollY;
        for (const auto& line : terminalBuffer) {
            sf::Text tText(font, line.text, 15); tText.setFillColor(line.color); tText.setPosition(sf::Vector2f(10.f, tY));
            window.draw(tText);
            int nl = 1; for(char c : line.text) if (c=='\n') nl++;
            tY += 18.f * nl;
        }

        // --- BAR CODES BACK TO RASTER SCAN DIRECT VIEWS ---
        window.setView(window.getDefaultView());
        window.draw(vertSplitter); window.draw(horizSplitter);
        window.draw(btnRun); window.draw(textRun);
        window.draw(btnNew); window.draw(textNew);
        window.draw(btnClear); window.draw(textClear);

        // Inline terminal clear button rendering macro
        window.draw(btnTermClear); window.draw(textTermClear);

        sf::Text sidebarHeaderText(font, "EXPLORER DASHBOARD\n\nv WORKING_TREE", 14);
        sidebarHeaderText.setFillColor(sf::Color(140, 142, 145));
        sidebarHeaderText.setPosition(sf::Vector2f(12.f, 60.f));
        window.draw(sidebarHeaderText);

        float fileY = 125.f;
        for (auto const& [fileName, fileContent] : files) {
            sf::Text fileItem(font, "  [f] " + fileName, 15); fileItem.setPosition(sf::Vector2f(12.f, fileY));
            fileItem.setFillColor(fileName == currentFile ? sf::Color(100, 200, 255) : sf::Color(140, 140, 140));
            window.draw(fileItem);
            fileY += 25.f;
        }

        int lines = 1; for (char c : files[currentFile]) if (c == '\n') lines++;
        std::string lineStr = ""; for (int i = 1; i <= lines; i++) lineStr += std::to_string(i) + "\n";
        sf::Text lineNumsDisplay(font, lineStr, 18);
        lineNumsDisplay.setFillColor(sf::Color(90, 95, 100)); // Distinct dark contrast numbers
        lineNumsDisplay.setPosition(sf::Vector2f(splitVertical + 10.f, 60.f - editorScrollY));
        window.draw(lineNumsDisplay);

        // --- CRITICAL OVERLAY PANEL CONTAINER ---
        if (isModalOpen) {
            sf::RectangleShape modalDim(sf::Vector2f(W, H)); modalDim.setFillColor(sf::Color(0, 0, 0, 180)); window.draw(modalDim);
            sf::RectangleShape modalBox(sf::Vector2f(450.f, 150.f)); modalBox.setPosition(sf::Vector2f(W/2 - 225.f, H/2 - 75.f)); modalBox.setFillColor(sf::Color(35, 35, 35)); modalBox.setOutlineThickness(2.f); modalBox.setOutlineColor(sf::Color(120, 120, 120)); window.draw(modalBox);
            sf::Text title(font, "Create File (Right-Click/Ctrl+V Allowed):", 15); title.setPosition(sf::Vector2f(W/2 - 200.f, H/2 - 50.f)); window.draw(title);
            sf::RectangleShape inputBox(sf::Vector2f(410.f, 32.f)); inputBox.setPosition(sf::Vector2f(W/2 - 205.f, H/2 - 15.f)); inputBox.setFillColor(sf::Color(20, 20, 20)); window.draw(inputBox);
            sf::Text input(font, modalInput + "_", 18); input.setPosition(sf::Vector2f(W/2 - 195.f, H/2 - 10.f)); window.draw(input);
        }
        window.display();
    }
    return 0;
}
