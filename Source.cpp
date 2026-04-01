#include <iostream>
#include <SFML/Graphics.hpp>

#include "Timer.hpp"

using namespace sf;

enum Direction
{
	Right = 0,
	Down,
	Left,
	Up
};

enum RelativeDirection
{
	LeftHand = -1,
	Straight,
	RightHand,
	Back
};

static RelativeDirection& operator++(RelativeDirection& direction)
{
	return direction = static_cast<RelativeDirection>(static_cast<int>(direction) + 1);
}

namespace std {
	template <>
	struct hash<Color> {
		size_t operator()(const Color& color) const {
			return color.r + 256 * color.g + 65536 * color.b;
		}
	};
}

int main(int argc, char** argv)
{
	Image mask;
	std::string maskFileName = "mask.png";
	Image colorMask;
	std::string colorMaskFileName = "color_mask.png";

	if (argc == 3)
	{
		maskFileName = argv[1];
		colorMaskFileName = argv[2];
	}

	if (!mask.loadFromFile(maskFileName))
	{
		std::cerr << "Could not open \"" + maskFileName + "\"!\n";
		return EXIT_FAILURE;
	}

	if (!colorMask.loadFromFile(colorMaskFileName))
	{
		std::cerr << "Could not open \"" + colorMaskFileName + "\"!\n";
		return EXIT_FAILURE;
	}

	Timer timer;

	const auto [maskWidth, maskHeight] = mask.getSize();

	for (uint32_t row = 0; row < maskHeight; ++row)
	{
		for (uint32_t column = 0; column < maskWidth; ++column)
		{
			if (mask.getPixel({ column, row }) != Color::White)
				continue;

			Vector2u traversingPosition{ column, row };
			auto& [traversingColumn, traversingRow] = traversingPosition;
			Direction traversingDirection = Direction::Right;

			std::vector<std::vector<uint32_t>> linesToFill(1);
			linesToFill[0].push_back(traversingColumn);
			uint32_t relativeY = 0;
			uint32_t maximumRelativeY = 0;

			auto nextPosition = [&](const Direction directionToCheck) -> std::optional<Vector2u>
				{
					switch (directionToCheck)
					{
					case Right:
						if (traversingColumn == maskWidth - 1)
							return std::nullopt;
						break;
					case Down:
						if (traversingRow == maskHeight - 1)
							return std::nullopt;
						break;
					case Left:
						if (traversingColumn == 0)
							return std::nullopt;
						break;
					case Up:
						if (traversingRow == 0)
							return std::nullopt;
						break;
					}

					const Vector2u positionToCheck{ traversingColumn + (directionToCheck % 2 == 0) - 2 * (directionToCheck == Direction::Left),
													traversingRow + (directionToCheck % 2) - 2 * (directionToCheck == Direction::Up) };

					if (mask.getPixel(positionToCheck) == Color::White)
						return positionToCheck;
					else
						return std::nullopt;
				};

			do // Traversing around the area edge and gathering information
			{
				for (RelativeDirection relativeDirection = RelativeDirection::LeftHand; relativeDirection < 3; ++relativeDirection)
				{
					const Direction possibleNextDirection = static_cast<Direction>((4 + (traversingDirection + relativeDirection) % 4) % 4);
					const auto possibleNextPosition = nextPosition(possibleNextDirection);
					if (possibleNextPosition.has_value())
					{
						if (relativeDirection != RelativeDirection::LeftHand)
						{
							if (traversingDirection == Direction::Down || traversingDirection == Direction::Up)
							{
								linesToFill[relativeY].push_back(traversingColumn);
								if (relativeDirection == RelativeDirection::Back)
									linesToFill[relativeY].push_back(traversingColumn);
							}
							else if (possibleNextDirection == Direction::Down || possibleNextDirection == Direction::Up || relativeDirection == RelativeDirection::Back)
								linesToFill[relativeY].push_back(traversingColumn);
						}

						traversingDirection = possibleNextDirection;
						traversingPosition = possibleNextPosition.value();

						if (traversingDirection == Direction::Down)
						{
							if (++relativeY > maximumRelativeY)
							{
								maximumRelativeY = relativeY;
								linesToFill.push_back({});
							}

						}
						else if (traversingDirection == Direction::Up)
						{
							--relativeY;
						}

						break;
					}
				}
			} while (traversingRow != row || traversingColumn != column);

			std::unordered_map<Color, uint32_t> pixelColorCount(linesToFill.size());

			for (uint32_t lineRow = 0; lineRow < linesToFill.size(); ++lineRow) //Can be parallelized for large and/or complex shapes 
			{
				if (linesToFill[lineRow].size() > 1)
				{
					std::sort(linesToFill[lineRow].begin(), linesToFill[lineRow].end());
					for (uint32_t linePartStartColumn = 0; linePartStartColumn < linesToFill[lineRow].size(); linePartStartColumn += 2)
					{
						for (uint32_t pixelColumn = linesToFill[lineRow][linePartStartColumn]; pixelColumn <= linesToFill[lineRow][linePartStartColumn + 1]; ++pixelColumn)
						{
							const Color pixel = colorMask.getPixel({ pixelColumn, row + lineRow });
							pixelColorCount[pixel]++;
						}
					}
				}
				else
				{
					linesToFill[lineRow].push_back(linesToFill[lineRow].back()); //If area is 1 pixel
				}
			}

			Color mostFrequentColor;
			uint32_t maxColorFrequency = 1;

			for (const auto& [color, frequency] : pixelColorCount)
			{
				if (frequency > maxColorFrequency)
				{
					mostFrequentColor = color;
					maxColorFrequency = frequency;
				}
			}

			for (uint32_t lineRow = 0; lineRow < linesToFill.size(); ++lineRow) //Can also be parallelized
				for (uint32_t linePartStartColumn = 0; linePartStartColumn < linesToFill[lineRow].size(); linePartStartColumn += 2)
					for (uint32_t pixelColumn = linesToFill[lineRow][linePartStartColumn]; pixelColumn <= linesToFill[lineRow][linePartStartColumn + 1]; ++pixelColumn)
						mask.setPixel({ pixelColumn, row + lineRow }, mostFrequentColor);

			column = linesToFill[0][1]; // Move column index to safely skip a part of processed shape
		}
	}

	std::cout << "Processing completed. Time: " << timer.time_elapsed().count() << "ms.\n";

	if (mask.saveToFile(maskFileName))
	{
		std::cout << "Write to file completed.\n";
		return EXIT_SUCCESS;
	}
	else
	{
		std::cerr << "Could not write to file!\n";
		return EXIT_FAILURE;
	}
}