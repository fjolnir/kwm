#include "kwm.h"

extern screen_info *Screen;
extern std::vector<std::string> FloatingAppLst;
extern window_info *FocusedWindow;
extern focus_option KwmFocusMode;
extern space_tiling_option KwmSpaceMode;
extern int KwmSplitMode;
extern bool KwmUseMouseFollowsFocus;
extern bool KwmEnableTilingMode;
extern bool KwmUseBuiltinHotkeys;
extern bool KwmEnableDragAndDrop;
extern bool KwmUseContextMenuFix;

std::string CreateStringFromTokens(std::vector<std::string> Tokens, int StartIndex)
{
    std::string Text = "";
    int TokenIndex = StartIndex;

    for(; TokenIndex < Tokens.size(); ++TokenIndex)
    {
        Text += Tokens[TokenIndex];
        if(TokenIndex < Tokens.size() - 1)
            Text += " ";
    }

    return Text;
}

std::vector<std::string> SplitString(std::string Line, char Delim)
{
    std::vector<std::string> Elements;
    std::stringstream Stream(Line);
    std::string Temp;

    while(std::getline(Stream, Temp, Delim))
        Elements.push_back(Temp);

    return Elements;
}

void KwmInterpretCommand(std::string Message, int ClientSockFD)
{
    std::vector<std::string> Tokens = SplitString(Message, ' ');

    if(Tokens[0] == "quit")
    {
        KwmQuit();
    }
    else if(Tokens[0] == "config")
    {
        if(Tokens[1] == "reload")
        {
            KwmReloadConfig();
        }
        else if(Tokens[1] == "prefix")
        {
            KwmSetGlobalPrefix(Tokens[2]);
        }
        else if(Tokens[1] == "prefix-timeout")
        {
            double Value = 0;
            std::stringstream Stream(Tokens[2]);
            Stream >> Value;
            KwmSetGlobalPrefixTimeout(Value);
        }
        else  if(Tokens[1] == "launchd")
        {
            if(Tokens[2] == "disable")
                RemoveKwmFromLaunchd();
            else if(Tokens[2] == "enable")
                AddKwmToLaunchd();
        }
        else if(Tokens[1] == "tiling")
        {
            if(Tokens[2] == "disable")
                KwmEnableTilingMode = false;
            else if(Tokens[2] == "enable")
                KwmEnableTilingMode = true;
        }
        else if(Tokens[1] == "space")
        {
            if(Tokens[2] == "bsp")
                KwmSpaceMode = SpaceModeBSP;
            else if(Tokens[2] == "monocle")
                KwmSpaceMode = SpaceModeMonocle;
            else if(Tokens[2] == "float")
                KwmSpaceMode = SpaceModeFloating;
        }
        else if(Tokens[1] == "focus")
        {
            if(Tokens[2] == "mouse-follows")
            {
                if(Tokens[3] == "disable")
                    KwmUseMouseFollowsFocus = false;
                else if(Tokens[3] == "enable")
                    KwmUseMouseFollowsFocus = true;
            }
            else if(Tokens[2] == "toggle")
            {
                if(KwmFocusMode == FocusModeDisabled)
                    KwmFocusMode = FocusModeAutofocus;
                else if(KwmFocusMode == FocusModeAutofocus)
                    KwmFocusMode = FocusModeAutoraise;
                else if(KwmFocusMode == FocusModeAutoraise)
                    KwmFocusMode = FocusModeDisabled;
            }
            else if(Tokens[2] == "autofocus")
                KwmFocusMode = FocusModeAutofocus;
            else if(Tokens[2] == "autoraise")
                KwmFocusMode = FocusModeAutoraise;
            else if(Tokens[2] == "disabled")
                KwmFocusMode = FocusModeDisabled;
        }
        else if(Tokens[1] == "hotkeys")
        {
            if(Tokens[2] == "disable")
                KwmUseBuiltinHotkeys = false;
            else if(Tokens[2] == "enable")
                KwmUseBuiltinHotkeys = true;
        }
        else if(Tokens[1] == "dragndrop")
        {
            if(Tokens[2] == "disable")
                KwmEnableDragAndDrop = false;
            else if(Tokens[2] == "enable")
                KwmEnableDragAndDrop = true;
        }
        else if(Tokens[1] == "menu-fix")
        {
            if(Tokens[2] == "disable")
                KwmUseContextMenuFix = false;
            else if(Tokens[2] == "enable")
                KwmUseContextMenuFix = true;
        }
        else if(Tokens[1] == "float")
        {
            FloatingAppLst.push_back(CreateStringFromTokens(Tokens, 2));
        }
        else if(Tokens[1] == "add-role")
        {
            AllowRoleForApplication(CreateStringFromTokens(Tokens, 3), Tokens[2]);
        }
        else if(Tokens[1] == "padding")
        {
            if(Tokens[2] == "left" || Tokens[2] == "right" ||
                    Tokens[2] == "top" || Tokens[2] == "bottom")
            {
                int Value = 0;
                std::stringstream Stream(Tokens[3]);
                Stream >> Value;
                SetDefaultPaddingOfDisplay(Tokens[2], Value);
            }
        }
        else if(Tokens[1] == "gap")
        {
            if(Tokens[2] == "vertical" || Tokens[2] == "horizontal")
            {
                int Value = 0;
                std::stringstream Stream(Tokens[3]);
                Stream >> Value;
                SetDefaultGapOfDisplay(Tokens[2], Value);
            }
        }
        if(Tokens[1] == "split-ratio")
        {
            double Value = 0;
            std::stringstream Stream(Tokens[2]);
            Stream >> Value;
            ChangeSplitRatio(Value);
        }
    }
    else if(Tokens[0] == "focused")
    {
        std::string Output;
        GetTagForCurrentSpace(Output);

        if(FocusedWindow)
            Output += " " + FocusedWindow->Owner + " - " + FocusedWindow->Name;

        KwmWriteToSocket(ClientSockFD, Output);
    }
    else if(Tokens[0] == "window")
    {
        if(Tokens[1] == "-t")
        {
            if(Tokens[2] == "fullscreen")
                ToggleFocusedWindowFullscreen();
            else if(Tokens[2] == "parent")
                ToggleFocusedWindowParentContainer();
            else if(Tokens[2] == "float")
                ToggleFocusedWindowFloating();
            else if(Tokens[2] == "mark")
                MarkWindowContainer();
        }
        else if(Tokens[1] == "-c")
        {
            if(Tokens[2] == "split")
            {
                space_info *Space = &Screen->Space[Screen->ActiveSpace];
                tree_node *Node = GetNodeFromWindowID(Space->RootNode, FocusedWindow->WID, Space->Mode);
                ToggleNodeSplitMode(Screen, Node->Parent);
            }
            else if(Tokens[2] == "reduce" || Tokens[2] == "expand")
            {
                double Ratio = 0.1;
                std::stringstream Stream(Tokens[3]);
                Stream >> Ratio;

                if(Tokens[2] == "reduce")
                    ModifyContainerSplitRatio(-Ratio);
                else if(Tokens[2] == "expand")
                    ModifyContainerSplitRatio(Ratio);
            }
            else if(Tokens[2] == "refresh")
            {
                ResizeWindowToContainerSize();
            }
        }
        else if(Tokens[1] == "-f")
        {
            if(Tokens[2] == "prev")
                ShiftWindowFocus(-1);
            else if(Tokens[2] == "next")
                ShiftWindowFocus(1);
            else if(Tokens[2] == "curr")
                FocusWindowBelowCursor();
        }
        else if(Tokens[1] == "-s")
        {
            if(Tokens[2] == "prev")
                SwapFocusedWindowWithNearest(-1);
            else if(Tokens[2] == "next")
                SwapFocusedWindowWithNearest(1);
            else if(Tokens[2] == "mark")
                SwapFocusedWindowWithMarked();
        }
    }
    else if(Tokens[0] == "screen")
    {
        if(Tokens[1] == "-s")
        {
            if(Tokens[2] == "optimal")
                KwmSplitMode = -1;
            else if(Tokens[2] == "vertical")
                KwmSplitMode = 1;
            else if(Tokens[2] == "horizontal")
                KwmSplitMode = 2;
        }
        else if(Tokens[1] == "-m")
        {
            if(Tokens[2] == "prev")
                CycleFocusedWindowDisplay(-1, true);
            else if(Tokens[2] == "next")
                CycleFocusedWindowDisplay(1, true);
            else
            {
                int Index = 0;
                std::stringstream Stream(Tokens[2]);
                Stream >> Index;
                CycleFocusedWindowDisplay(Index, false);
            }
        }
    }
    else if(Tokens[0] == "space")
    {
        if(Tokens[1] == "-t")
        {
            if(Tokens[2] == "toggle")
                ToggleFocusedSpaceFloating();
            else if(Tokens[2] == "float")
                FloatFocusedSpace();
            else if(Tokens[2] == "bsp")
                TileFocusedSpace(SpaceModeBSP);
            else if(Tokens[2] == "monocle")
                TileFocusedSpace(SpaceModeMonocle);
        }
        else if(Tokens[1] == "-r")
        {
            if(Tokens[2] == "90" || Tokens[2] == "270" || Tokens[2] == "180")
            {
                int Deg = 0;
                std::stringstream Stream(Tokens[2]);
                Stream >> Deg;

                space_info *Space = &Screen->Space[Screen->ActiveSpace];
                if(Space->Mode == SpaceModeBSP)
                {
                    RotateTree(Space->RootNode, Deg);
                    CreateNodeContainers(Screen, Space->RootNode, false);
                    ApplyNodeContainer(Space->RootNode, Space->Mode);
                }
            }
        }
        else if(Tokens[1] == "-p")
        {
            if(Tokens[3] == "left" || Tokens[3] == "right" ||
                    Tokens[3] == "top" || Tokens[3] == "bottom")
            {
                int Value = 0;
                if(Tokens[2] == "increase")
                    Value = 10;
                else if(Tokens[2] == "decrease")
                    Value = -10;

                ChangePaddingOfDisplay(Tokens[3], Value);
            }

        }
        else if(Tokens[1] == "-g")
        {
            if(Tokens[3] == "vertical" || Tokens[3] == "horizontal")
            {
                int Value = 0;
                if(Tokens[2] == "increase")
                    Value = 10;
                else if(Tokens[2] == "decrease")
                    Value = -10;

                ChangeGapOfDisplay(Tokens[3], Value);
            }
        }
    }
    else if(Tokens[0] == "write")
    {
        KwmEmitKeystrokes(CreateStringFromTokens(Tokens, 1));
    }
    else if(Tokens[0] == "bind")
    {
        if(Tokens.size() > 2)
            KwmAddHotkey(Tokens[1], CreateStringFromTokens(Tokens, 2));
        else
            KwmAddHotkey(Tokens[1], "");
    }
    else if(Tokens[0] == "unbind")
    {
        KwmRemoveHotkey(Tokens[1]);
    }
}
