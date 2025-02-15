﻿// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#include "pch.h"
#include<future>
#include<format>
#include<windows.ui.h>

#include<windows.ui.xaml.h>
#include<windows.ui.xaml.media.h>
#include<winrt/Microsoft.Graphics.Canvas.h>


#include "MainWindow.xaml.h"

#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::ModernLife::implementation
{
    MainWindow::MainWindow()
    {
        InitializeComponent();

        StartGameLoop();
    }

    void MainWindow::OnTick(IInspectable const& sender, IInspectable const& event)
    {
        sender;
        event;

        {
            std::scoped_lock lock{ lockboard };
            board.ConwayUpdateBoardWithNextState();
            board.ApplyNextStateToBoard();
        }
        theCanvas().Invalidate();
    }

    void MainWindow::StartGameLoop()
    {
        // create the board
        board = Board{ cellcount, cellcount };

        auto randomizer = std::async(&Board::RandomizeBoard, &board, _randompercent / 100.0f);
        randomizer.wait();

        // create and start a timer
        _controller = DispatcherQueueController::CreateOnDedicatedThread();
        _queue = _controller.DispatcherQueue();
        _timer = _queue.CreateTimer();
        using namespace  std::literals::chrono_literals;
        _timer.Interval(std::chrono::milliseconds{ 16 });
        _timer.IsRepeating(true);
        auto registrationtoken = _timer.Tick({ this, &MainWindow::OnTick });

        using namespace Microsoft::UI::Xaml::Controls;
        GoButton().Icon(SymbolIcon(Symbol::Play));
        GoButton().Label(L"Play");

        theCanvas().Invalidate();
    }

    void MainWindow::CanvasControl_Draw(CanvasControl  const& sender, CanvasDrawEventArgs const& args)
    {
        RenderOffscreen(sender);
        {
            std::scoped_lock lock{ lockbackbuffer };
            args.DrawingSession().DrawImage(_back, 0, 0);
        }
    }

    Windows::UI::Color MainWindow::GetCellColor2(const Cell& cell)
    {
        if (!_colorinit)
        {
            for (int index = 0; index <= maxage + 1; index++)
            {
                int a = 255;
                int r = (index * 255)/maxage;
                int g = 128;
                int b = 128;
                vecColors.emplace_back(ColorHelper::FromArgb(a, r, g, b));

            }

            // setup vector of colors
            _colorinit = true;
        }
        int age = cell.Age() > maxage ? maxage : cell.Age();

        return vecColors[age];
    }

    Windows::UI::Color MainWindow::GetCellColor3(const Cell& cell)
    {
        if (!_colorinit)
        {
            float h = 0.0f;

            vecColors.resize(maxage + 1);
            for (int index = 100; index < maxage - 100 + 1; index++)
            {
                h = ((float)index) / maxage * 360.0f;
                vecColors[index] = HSVtoRGB2(h, 60.0, 60.0);
            }
            _colorinit = true;
        }

        if (cell.Age() < 100)
        {
            return Windows::UI::Colors::Green();
        }
        
        if (cell.Age() > maxage - 100)
        {
            return Windows::UI::Colors::Black();
        }

        int age = cell.Age() > maxage ? maxage : cell.Age();

        return vecColors[age];
    }

    Windows::UI::Color MainWindow::GetCellColor(const Cell& cell) const
    {
        uint8_t colorscale = 0;
        uint8_t red = 0;
        uint8_t green = 0;
        uint8_t blue = 0;

        colorscale = (cell.Age() * 255) / maxage;
        colorscale = 254 - colorscale;

        if (cell.Age() <= (maxage * 1/4))
        {
            green = colorscale;
        }
        else if (cell.Age() > (maxage * 1/4) && cell.Age() <= (maxage * 1/2))
        {
            blue = colorscale;
        }
        else if (cell.Age() > (maxage * 1/2) && cell.Age() <= (maxage * 3/4))
        {
            red = colorscale;
        }
        else if (cell.Age() > (maxage * 3/4) && cell.Age() <= maxage)
        {
            red = colorscale;
            green = colorscale;
            blue = colorscale;
        }
        Windows::UI::Color cellcolor = ColorHelper::FromArgb(255, red, green, blue);
        return cellcolor;
    }

    void MainWindow::DrawInto(CanvasDrawingSession& ds, int startY, int endY, float width)
	{
        //float inc = width / cellcount;
        //if (drawgrid)
		//{
		//	for (int i = 0; i <= cellcount; i++)
		//	{
		//		ds.DrawLine(0, i * inc, height, i * inc, Colors::DarkSlateGray());
		//		ds.DrawLine(i * inc, 0, i * inc, width, Colors::DarkSlateGray());
		//	}
		//}

        float w = (width / cellcount);
		float posx = 0.0f;
		float posy = startY * w;
        {
            std::scoped_lock lock{ lockboard };

            for (int y = startY; y < endY; y++)
            {
                for (int x = 0; x < board.Width(); x++)
                {
                    if (const Cell& cell = board.GetCell(x, y); cell.IsAlive())
                    {
                        ds.DrawRoundedRectangle(posx, posy, w, w, 2, 2, GetCellColor3(cell));
                    }
                    posx += w;
                }
                posy += w;
                posx = 1.0f;
            }
        }
	}

    void MainWindow::RenderOffscreen(CanvasControl const& sender)
    {
        // https://microsoft.github.io/Win2D/WinUI2/html/Offscreen.htm

        if (!board.IsDirty())
            return;

        constexpr int bestsize = cellcount * 8;
        winrt::Windows::Foundation::Size huge = sender.Size();
        float width = min(huge.Width, bestsize);
        float height = (width/cellcount) * board.Height();

        // if the back buffer doesn't exist or is the wrong size, create it
        if (nullptr == _back || _back.Size() != sender.Size())
        {
            CanvasDevice device = CanvasDevice::GetSharedDevice();
            {
                std::scoped_lock lock{ lockbackbuffer };
                _back = CanvasRenderTarget(device, width, height, sender.Dpi());
            }
        }

        CanvasDrawingSession ds = _back.CreateDrawingSession();

        using namespace Microsoft::UI::Xaml::Controls;
        using namespace Microsoft::UI::Xaml::Media;
        Brush backBrush{ splitView().PaneBackground() };
        SolidColorBrush scbBack = backBrush.try_as<SolidColorBrush>();
        Windows::UI::Color colorBack{scbBack.Color()};

        ds.FillRectangle(0, 0, width, height, Colors::WhiteSmoke());

        if (singlerenderer)
        {
            // render in one thread
            auto drawinto0 = std::async(&MainWindow::DrawInto, this, std::ref(ds), 0, board.Height(), _back.Size().Width);
            drawinto0.wait();
        }
        else
        {
            // render in 4 threads
            auto drawinto1 = std::async(&MainWindow::DrawInto, this, std::ref(ds), 0,                    board.Height() * 1/4, _back.Size().Width);
            auto drawinto2 = std::async(&MainWindow::DrawInto, this, std::ref(ds), board.Height() * 1/4, board.Height() * 1/2, _back.Size().Width);
            auto drawinto3 = std::async(&MainWindow::DrawInto, this, std::ref(ds), board.Height() * 1/2, board.Height() * 3/4, _back.Size().Width);
            auto drawinto4 = std::async(&MainWindow::DrawInto, this, std::ref(ds), board.Height() * 3/4, board.Height(),       _back.Size().Width);

            drawinto1.wait();
            drawinto2.wait();
            drawinto3.wait();
            drawinto4.wait();
        }
    }

    int32_t MainWindow::SeedPercent() const
    {
        return _randompercent;
    }

    void MainWindow::SeedPercent(int32_t value)
    {
        if (_randompercent != value)
        {
            _randompercent = value;

            m_propertyChanged(*this, PropertyChangedEventArgs{ L"SeedPercent" });
        }
    }

    void MainWindow::MyProperty(int32_t /* value */)
    {
        throw hresult_not_implemented();
    }

    int32_t MainWindow::MyProperty()
    {
        throw hresult_not_implemented();
    }

    void MainWindow::theCanvasDebug_Draw(winrt::Microsoft::Graphics::Canvas::UI::Xaml::CanvasControl const& sender, winrt::Microsoft::Graphics::Canvas::UI::Xaml::CanvasDrawEventArgs const& args)
    {
        sender;
        
        using namespace Microsoft::UI::Xaml::Controls;
        using namespace Microsoft::UI::Xaml::Media;

        Microsoft::Graphics::Canvas::Text::CanvasTextFormat canvasFmt{};
        canvasFmt.FontFamily(PaneHeader().FontFamily().Source());
        canvasFmt.FontSize(PaneHeader().FontSize());

        Brush backBrush{ splitView().PaneBackground() };
        Brush textBrush{ PaneHeader().Foreground() };

        SolidColorBrush scbBack = backBrush.try_as<SolidColorBrush>();
        SolidColorBrush scbText = textBrush.try_as<SolidColorBrush>();

        Windows::UI::Color colorBack{scbBack.Color()};
        Windows::UI::Color colorText{scbText.Color()};

        args.DrawingSession().Clear(colorBack);

        std::wstring str = std::format(L"Generation\t{}\r\nAlive\t\t{}\r\nTotal Cells\t{}", board.Generation(), board.GetLiveCount(), board.GetSize());
        sender.Invalidate();

        args.DrawingSession().DrawText(str, 0, 0, 200, 200, colorText, canvasFmt);
    }

    void MainWindow::GoButton_Click(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        e;
        sender;

        using namespace Microsoft::UI::Xaml::Controls;
        if (_timer.IsRunning())
        {
            GoButton().Label(L"Play");
            GoButton().Icon(SymbolIcon(Symbol::Play));
            _timer.Stop();

        }
        else
        {
            GoButton().Icon(SymbolIcon(Symbol::Pause));
            GoButton().Label(L"Pause");
            _timer.Start();
        }
    }

    void MainWindow::RestartButton_Click(IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        e;
        sender;

        using namespace Microsoft::UI::Xaml::Controls;

        _timer.Stop();
        GoButton().Icon(SymbolIcon(Symbol::Pause));
        GoButton().Label(L"Pause");
        StartGameLoop();
    }

    hstring MainWindow::GetSliderText(int32_t value)
    {
        // only called once when the app starts
        std::wstring slidertext = std::format(L"{0}% random", sliderPop().Value());
        hstring hslidertext{ slidertext };
        return hslidertext;
    }

    void MainWindow::sliderPop_ValueChanged(IInspectable const& sender, winrt::Microsoft::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const& e)
    {
        // old way, this works
        //std::wstring slidertext = std::format(L"{0}% random", sliderPop().Value());

        //if (nullptr != popSliderText())
        //{
        //    popSliderText().Text(slidertext);
        //}
    }

    Windows::UI::Color MainWindow::HSVtoRGB2(float H, float S, float V)
    {
        float s = S / 100;
        float v = V / 100;
        float C = s * v;
        float X = C * (1 - abs(fmod(H / 60.0, 2) - 1));
        float m = v - C;
        float r = 0;
        float g = 0;
        float b = 0;

        if (H >= 0 && H < 60) {
            r = C, g = X, b = 0;
        }
        else if (H >= 60 && H < 120) {
            r = X, g = C, b = 0;
        }
        else if (H >= 120 && H < 180) {
            r = 0, g = C, b = X;
        }
        else if (H >= 180 && H < 240) {
            r = 0, g = X, b = C;
        }
        else if (H >= 240 && H < 300) {
            r = X, g = 0, b = C;
        }
        else {
            r = C, g = 0, b = X;
        }
        int R = (r + m) * 255;
        int G = (g + m) * 255;
        int B = (b + m) * 255;
    
        return ColorHelper::FromArgb(255, R, G, B);
    }
}




