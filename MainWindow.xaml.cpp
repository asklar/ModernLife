﻿// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#include "pch.h"
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif
//#include <future>

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::ModernLife::implementation
{
    //auto C = std::bind_front(&Board::ConwayRules, &board);
    //auto D = std::bind_front(&Board::DayAndNightRules, &board);
    //auto S = std::bind_front(&Board::SeedsRules, &board);
    //auto B = std::bind_front(&Board::BriansBrainRules, &board);
    //auto H = std::bind_front(&Board::HighlifeRules, &board);
    //auto L = std::bind_front(&Board::LifeWithoutDeathRules, &board);
    bool drawgrid = false;

    MainWindow::MainWindow()
    {
        InitializeComponent();
        //board = Board{ cellcount, cellcount };

        int n = board.Width() * board.Height() / 4;
        board.RandomizeBoard(n);

        CanvasDevice device = CanvasDevice::GetSharedDevice();
        back = CanvasRenderTarget{ device, 2000, 1000, 96 };
    }

    int32_t MainWindow::MyProperty()
    {
        throw hresult_not_implemented();
    }

    void MainWindow::MyProperty(int32_t /* value */)
    {
        throw hresult_not_implemented();
    }

    void MainWindow::CanvasControl_Draw(CanvasControl  const& sender, CanvasDrawEventArgs const& args)
    {
        RenderOffscreen(sender);
        args.DrawingSession().DrawImage(back, 0, 0);
    }

    void MainWindow::RenderOffscreen(CanvasControl const& sender)
    {
        // https://microsoft.github.io/Win2D/WinUI2/html/Offscreen.htm

        //CanvasDrawingSession ds = back.CreateDrawingSession();
        //CanvasDevice device = CanvasDevice::GetSharedDevice();
        //CanvasRenderTarget flip{ device, (float)sender.Width(), (float)sender.Height(), sender.Dpi()};

        //back = flip;// CanvasRenderTarget{ device, (float)sender.Width(), (float)sender.Height(), sender.Dpi() };
        CanvasDrawingSession ds = back.CreateDrawingSession();

        ds.Clear(Colors::WhiteSmoke());

        winrt::Windows::Foundation::Size huge = sender.Size();
        float inc = huge.Width / cellcount;

        if (drawgrid)
        {
            for (int i = 0; i <= cellcount; i++)
            {
                ds.DrawLine(0, i * inc, huge.Height, i * inc, Colors::DarkSlateGray());
                ds.DrawLine(i * inc, 0, i * inc, huge.Width, Colors::DarkSlateGray());
            }
        }

        float w = (huge.Width / cellcount) - 2;

        float posx = 1.0f;
        float posy = 1.0f;
        for (int y = 0; y < cellcount; y++)
        {
            for (int x = 0; x < cellcount; x++)
            {
                const Cell& cell = board.GetCell(x, y);
                if (cell.IsAlive())
                {
                    auto cellcolor = Colors::Black();
                    if (cell.Age() < 1)
                    {
                        cellcolor = Colors::Green();
                    }

                    ds.DrawRoundedRectangle(posx, posy, w, w, 2, 2, cellcolor);
                }
                posx += w;
            }
            posy += w;
            posx = 1.0f;
        }

        {
            // auto theasync=std::async([&p,i]{ return p.sum(i);});
            // auto theasync = std::async([&board]{ return board.UpdateBoard(); });
            //std::async(Board::UpdateBoard, C);
            //std::async(Board::NextGeneration);
        }

            Sleep(50);
            auto C = std::bind_front(&Board::ConwayRules, &board);
            board.UpdateBoard(C);
            board.NextGeneration();
            sender.Invalidate();

        /*
        An app can close, and re-open drawing sessions on a CanvasRenderTarget abitrarily many times.
        Drawing operations are not committed to the CanvasRenderTarget until the drawing session object is disposed. In C#, a 'using' block can organize this.
        It's worth pointing out that CanvasRenderTarget is not a XAML control, and does not involve the XAML tree at all. It is suitable for both XAML and non-XAML-based apps.
        */
    }

}
