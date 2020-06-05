#pragma once
#include <iostream>
#include <vector>
#include <utility>
#include <algorithm>
#include <chrono>
#include <stdio.h>
#include <Windows.h>
#include <fstream>
using namespace std;

auto tp1 = chrono::system_clock::now(); // Рахування пройденого
auto tp2 = chrono::system_clock::now(); // часу

class RayCasting
{
	int nScreenWidth = 240;			// Екран
	int nScreenHeight = 80;

	const int nMapWidth = 16;	    // Світ
	const int nMapHeight = 16;

	float fPlayerX = 14.7f;			// Початкова позиція
	float fPlayerY = 5.09f;
	float fPlayerA = 0.0f;			// Початковий кут
	float fFOV = 3.14159f / 4.0f;	// Поле зору
	float fDepth = 16.0f;			// Максимальна видима дистанція
	float fSpeed = 5.0f;			// Швидкість руху
	float fElapsedTime = 0.0f;      // Витрачений час

	// Буфер екрану
	wchar_t* screen;
	HANDLE hConsole;
	DWORD dwBytesWritten;

	// Карта, . - вільна область, # - стіна
	wstring map = fillTheMap();
public:

	// Створення буферу екрану
	void createScreenBuffer()
	{
		screen = new wchar_t[nScreenWidth * nScreenHeight];
		hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
		SetConsoleActiveScreenBuffer(hConsole);
		dwBytesWritten = 0;
	}

	// Заповнення карти
	wstring fillTheMap()
	{
		wstring wMap, tempMap;
		wifstream f(L"map.txt");
		while (!f.eof())
		{
			f >> tempMap;
			wMap += tempMap;
		}
		f.close();
		return wMap;
	}

	// Стартовий екран
	void startChoice()
	{
		string command = "";
		while (command != "start")
		{
			cin >> command;
			if (command == "map")
				editMapFile();
		}
	}

	// Відрити файл з картою
	void editMapFile()
	{
		ShellExecute(NULL, L"open", L"map.txt", NULL, NULL, SW_SHOWNORMAL);
	}

	// Повороти гравця
	void turningAround()
	{
		// Поворот за годинниковою
		if (GetAsyncKeyState((unsigned short)'A') & 0x8000)
			fPlayerA -= (fSpeed * 0.75f) * fElapsedTime;

		// Поворот проти годинникової
		if (GetAsyncKeyState((unsigned short)'D') & 0x8000)
			fPlayerA += (fSpeed * 0.75f) * fElapsedTime;
	}

	// Рух гравця
	void movingForwardAndBackward()
	{
		// Рух вперед
		if (GetAsyncKeyState((unsigned short)'W') & 0x8000)
		{
			fPlayerX += sinf(fPlayerA) * fSpeed * fElapsedTime;;
			fPlayerY += cosf(fPlayerA) * fSpeed * fElapsedTime;;
			if (map.c_str()[(int)fPlayerX * nMapWidth + (int)fPlayerY] == '#') // Обробка зіткнень зі стінами
			{
				fPlayerX -= sinf(fPlayerA) * fSpeed * fElapsedTime;;
				fPlayerY -= cosf(fPlayerA) * fSpeed * fElapsedTime;;
			}
		}

		// Рух назад
		if (GetAsyncKeyState((unsigned short)'S') & 0x8000)
		{
			fPlayerX -= sinf(fPlayerA) * fSpeed * fElapsedTime;;
			fPlayerY -= cosf(fPlayerA) * fSpeed * fElapsedTime;;
			if (map.c_str()[(int)fPlayerX * nMapWidth + (int)fPlayerY] == '#') // Обробка зіткнень зі стінами
			{
				fPlayerX += sinf(fPlayerA) * fSpeed * fElapsedTime;;
				fPlayerY += cosf(fPlayerA) * fSpeed * fElapsedTime;;
			}
		}
	}

	// Мінікарта
	void showMiniMap()
	{
		for (int nx = 0; nx < nMapWidth; nx++)
			for (int ny = 0; ny < nMapWidth; ny++)
			{
				screen[(ny + 1) * nScreenWidth + nx] = map[ny * nMapWidth + nx];
			}
		screen[((int)fPlayerX + 1) * nScreenWidth + (int)fPlayerY] = 'P';
	}

	// Колір стінок, що залежить від відстані
	short drawWalls(float fDistanceToWall, bool bBoundary)
	{
		short nShade = ' ';
		if (fDistanceToWall <= fDepth / 4.0f)			nShade = 0x2588;	// Дуже близько	
		else if (fDistanceToWall < fDepth / 3.0f)		nShade = 0x2593;
		else if (fDistanceToWall < fDepth / 2.0f)		nShade = 0x2592;
		else if (fDistanceToWall < fDepth)				nShade = 0x2591;
		else											nShade = ' ';		// Дуже далеко

		if (bBoundary)nShade = ' '; // Затемняємо
		return nShade;
	}

	// Колір підлоги
	short drawFloor(float b, short nShade)
	{
		if (b < 0.25)	    return '#';
		else if (b < 0.5)	return 'x';
		else if (b < 0.75)	return '.';
		else if (b < 0.9)	return '-';
		else				return ' ';
	}

	// Ігровий цикл
	void gameProcess()
	{
		startChoice();
		fillTheMap();
		createScreenBuffer();
		while (1) // Ігровий цикл
		{
			// Використовуємо зміну часу для рахування модифікації
			// швидкостей руху, для забеспечення незмінної швидкості
			tp2 = chrono::system_clock::now();
			chrono::duration<float> elapsedTime = tp2 - tp1;
			tp1 = tp2;
			fElapsedTime = elapsedTime.count();

			turningAround();

			movingForwardAndBackward();

			for (int x = 0; x < nScreenWidth; x++)
			{
				float fRayAngle = (fPlayerA - fFOV / 2.0f) + ((float)x / (float)nScreenWidth) * fFOV; // Напрям променя
				// Відстань до стінки


				float fStepSize = 0.1f;		  // Інкремент для променів, збільшення =  зменшення стін									
				float fDistanceToWall = 0.0f; // Відстань до перешкоди по напряму fRayAngle

				bool bHitWall = false;		// Промінь досяг стінки
				bool bBoundary = false;		// Промінь досяг стик між стінками

				float fEyeX = sinf(fRayAngle); // Координати fRayAngle
				float fEyeY = cosf(fRayAngle);

				// Поки не зіткнулися зі стіною або не вийшли за радіус видимості
				while (!bHitWall && fDistanceToWall < fDepth)
				{
					fDistanceToWall += fStepSize;
					int nTestX = (int)(fPlayerX + fEyeX * fDistanceToWall); // Кінцеві координати
					int nTestY = (int)(fPlayerY + fEyeY * fDistanceToWall); // променя

					// Якщо вийшли за зону
					if (nTestX < 0 || nTestX >= nMapWidth || nTestY < 0 || nTestY >= nMapHeight)
					{
						bHitWall = true;
						fDistanceToWall = fDepth;
					}
					else
					{
						// Промінь входить, перевіряємо, чи кінцева точка це стіна
						if (map.c_str()[nTestX * nMapWidth + nTestY] == '#')
						{
							// Промінь потрапив у стіну
							bHitWall = true;

							vector<pair<float, float>> p;   // Вектор ребер

							// Проходимо по всім 4 ребрам
							for (int tx = 0; tx < 2; tx++)
								for (int ty = 0; ty < 2; ty++)
								{
									float vy = (float)nTestY + ty - fPlayerY; // Координати вектора, що
									float vx = (float)nTestX + tx - fPlayerX; // йде від спостерігача до ребра
									float d = sqrt(vx * vx + vy * vy); // Модуль цього вектора
									float dot = (fEyeX * vx / d) + (fEyeY * vy / d); // Скалярний добуток
									p.push_back(make_pair(d, dot)); // Зберігаємо результат у масив
								}

							// Сортуємо ребра по їх модулям векторів
							sort(p.begin(), p.end(), [](const pair<float, float>& left, const pair<float, float>& right) {return left.first < right.first; });

							// Перші 2/3 найближчі (4 побачити неможливо)
							float fBound = 0.01; // Кут, при якому починаємо бачити ребро
							if (acos(p.at(0).second) < fBound) bBoundary = true;
							if (acos(p.at(1).second) < fBound) bBoundary = true;
							if (acos(p.at(2).second) < fBound) bBoundary = true;
						}
					}
				}

				// Рахуємо відстань до стелі і підлоги
				int nCeiling = (float)(nScreenHeight / 2.0) - nScreenHeight / ((float)fDistanceToWall);
				int nFloor = nScreenHeight - nCeiling;

				// Замальовуємо стінки, в залежності від відстаней
				short nShade = drawWalls(fDistanceToWall, bBoundary);

				for (int y = 0; y < nScreenHeight; y++)
				{
					// Кожен ряд
					if (y <= nCeiling)
						screen[y * nScreenWidth + x] = ' ';
					else if (y > nCeiling && y <= nFloor)
						screen[y * nScreenWidth + x] = nShade;
					else // Підлога
					{
						// Замальовуємо підлогу, в залежності від відстані
						float b = 1.0f - (((float)y - nScreenHeight / 2.0f) / ((float)nScreenHeight / 2.0f));
						nShade = drawFloor(b, nShade);
						screen[y * nScreenWidth + x] = nShade;
					}
				}
			}

			// Статистика
			swprintf_s(screen, 40, L"X=%3.2f, Y=%3.2f, A=%3.2f FPS=%3.2f", fPlayerX, fPlayerY, fPlayerA, 1.0f / fElapsedTime);

			// Відображення мінікарти
			showMiniMap();

			// Відображення кадру
			screen[nScreenWidth * nScreenHeight - 1] = '\0';
			WriteConsoleOutputCharacter(hConsole, screen, nScreenWidth * nScreenHeight, { 0,0 }, &dwBytesWritten);
		}
	}
};