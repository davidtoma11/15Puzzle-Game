# 15 Puzzle Game using X11/Xlib

## 🎮 Game Description

The **15 Puzzle** is a classic sliding puzzle game consisting of a 4×4 grid with 15 numbered tiles and one empty space. The objective is to rearrange the tiles into numerical order by sliding them into the empty space.

### Key Features
- 🖱️ **Mouse-controlled gameplay**
- 🌓 **Dark/Light mode toggle**
- ⏱️ **Real-time timer**
- 📊 **Move counter**
- ✨ **Smooth tile animations**
- 🔄 **Reset game functionality**
  
<img src="15Game.png" alt="15 Puzzle Screenshot" width="350"/>

## 🚀 Quick Installation Guide (Linux)
```bash
sudo apt update && sudo apt install -y libx11-dev libxft-dev gcc make git && \
git clone https://github.com/davidtoma11/15Puzzle-Game.git && \
cd 15Puzzle-Game && make && ./game15
