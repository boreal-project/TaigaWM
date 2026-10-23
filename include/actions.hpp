#pragma once

enum class Action {
    None,
    SpawnSh,
    Close,
    FocusNext,
    Move,
    Resize,
    Exit,
    Fullscreen,
    Maximize,
    TagInc,
    WinTagInc,
    TagDec,
    WinTagDec,
    TagSet,
    WinTagSet,
    FocusMonNext,
    FocusMonPrev,
};
