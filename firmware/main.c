/*
 * Name: main.c
 * Author: Maarten van Ingen
 * Copyright: 2013, Maarten van Ingen
 * License: See LICENSE file
 * Description: Game of Life for Atmega
*/

#include <avr/io.h>
#include <stdlib.h>
#include <util/delay.h>
#include "spi.h"
#include "ili9320.h"

#define CELL_SIZE 10   // cell size on one side; 4 means a cell is 4x4 pixels
#define HORIZONTAL_SIZE 240 / CELL_SIZE
#define VERTICAL_SIZE 320 / CELL_SIZE
#define BACKGROUND_COLOR Black
#define CELL_COLOR Yellow
#define STARTING_CELLS 200

uint8_t nextGeneration[24][4]; // (x, y)
uint8_t currentGeneration[24][4]; // (x, y)

void Draw_Cell ( uint16_t x, uint16_t y )
{
    uint16_t i;
    uint16_t j;
    for ( i = CELL_SIZE * x; i < (CELL_SIZE * x) + CELL_SIZE; i++ )
    {
        for ( j = CELL_SIZE * y; j < (CELL_SIZE * y) + CELL_SIZE; j++ )
        {
            Display_SetPoint ( i, j, CELL_COLOR);
        }
    }
}

void Refresh_Cells( void )
{
    uint8_t x;
    uint8_t y;
    
    //Display_Clear ( BACKGROUND_COLOR );
    
    for ( x = 0; x < HORIZONTAL_SIZE; x++)
    {
        for ( y = 0; y < VERTICAL_SIZE; y++)
        {
            if ( (currentGeneration[x][y / 8] & (1 << (y % 8))) != (nextGeneration[x][y / 8] & (1 << (y % 8)))  )
            {
                // Okay something has changed so we must draw something.
                if ( nextGeneration[x][y / 8] & (1 << (y % 8))  )
                {
                //Draw_Cell ( x, y );
                    Display_DrawFilledRectangle ( x * CELL_SIZE, y * CELL_SIZE, x * CELL_SIZE + 10, y * CELL_SIZE + 10, CELL_COLOR );
                }
                else
                {
                    Display_DrawFilledRectangle ( x * CELL_SIZE, y * CELL_SIZE, x * CELL_SIZE + 10, y * CELL_SIZE + 10, BACKGROUND_COLOR );
                }
            }
        }
    }
    for ( x = 0; x < HORIZONTAL_SIZE; x++ )
    {
        for ( y = 0; y < 4; y++ )
        {
     
            currentGeneration[x][ y ] = nextGeneration [x][y];
            nextGeneration[x][y] = 0;
        }
    }
}



// Counts the living cells surrounding the given cell.
// Stops counting if 4 living cells are seen.
uint8_t WillCellLive ( uint16_t x, uint16_t y )
{
    // I think this is faster than a more neat way :-)
    uint8_t count = 0;
    uint8_t i,j;
    uint8_t iLive = 0;
    uint8_t max_X, max_Y, min_X, min_Y;
    max_X = max_Y = min_X = min_Y = 0;
    
    if ( x > 0 && x < HORIZONTAL_SIZE - 1 && y > 0 && y < VERTICAL_SIZE - 1 ) // this first, cause most cells should be here
    {
        min_X = x - 1;
        min_Y = y - 1;
        max_X = x + 1;
        max_Y = y + 1;
    }
    // Left side
    else if ( y > 0 && y < VERTICAL_SIZE && x == 0 )
    {
        min_X = x;
        min_Y = y - 1;
        max_X = x + 1;
        max_Y = y + 1;
    }
    // Right side
    else if ( y > 0 && y < VERTICAL_SIZE - 1 && x == HORIZONTAL_SIZE - 1 )
    {
        min_X = x - 1;
        min_Y = y - 1;
        max_X = x;
        max_Y = y + 1;
        
    }
    // Cells on the top
    else if ( x > 0 && x < HORIZONTAL_SIZE - 1 && y == 0 )
    {
        min_X = x - 1;
        min_Y = y;
        max_X = x + 1;
        max_Y = y + 1;
    }
    // Cells on the Bottom
    else if ( x > 0 && x < HORIZONTAL_SIZE - 1 && y == VERTICAL_SIZE - 1 )
    {
        min_X = x - 1;
        min_Y = y - 1;
        max_X = x + 1;
        max_Y = y;
    }
    // The corners
    else if ( x == 0 && y == 0)
    {
        min_X = x;
        min_Y = y;
        max_X = x + 1;
        max_Y = y + 1;
    }
    else if ( x == 0 && y == VERTICAL_SIZE - 1)
    {
        min_X = x;
        min_Y = y - 1;
        max_X = x + 1;
        max_Y = y;
    }
    else if ( x == HORIZONTAL_SIZE - 1 && y == 0)
    {
        min_X = x - 1;
        min_Y = y;
        max_X = x;
        max_Y = y + 1;
    }
    else if ( x == HORIZONTAL_SIZE - 1 && y == VERTICAL_SIZE - 1)
    {
        min_X = x - 1;
        min_Y = y - 1;
        max_X = x;
        max_Y = y;
    }
    for ( i = min_X; i <= max_X; i++)
    {
        for ( j = min_Y; j <= max_Y; j++)
        {
            // currentGeneration[x][y / 8] |= 1<<(y % 8);
            if ( (currentGeneration[i][j / 8] & (1 << (j % 8)) ) != 0x00 )
            {
                if ( j == y && i == x )
                {
                    // It's me and I'm alive!
                    iLive = 1;
                }
                else
                {
                    count += 1;
                }
                // This check may be more loading than leaving it out. Must rethink this one
                if ( count > 3 )
                {
                    return 0x00; // Too crowded; both living and dead cells will be dead next generation
                }
            }
        }
    }
    /*
     GoL rules:
     
     1. Any live cell with fewer than two live neighbours dies, as if caused by under-population.
     2. Any live cell with two or three live neighbours lives on to the next generation.
     3. Any live cell with more than three live neighbours dies, as if by overcrowding.
     4. Any dead cell with exactly three live neighbours becomes a live cell, as if by reproduction.

     */
    
    if ( count < 2)
    {
        return 0x00; // Fewer than 2 adjacent living cells, this cell dies or is kept dead
    }
    else if ( iLive && count == 2 )
    {
        return 0x01; // We keep on living
    }
    else if ( count == 3 )
    {
        return 0x01; // Dead cells will become a live one and living cells keep on living
    }
    else
    {
        /* This should account for every other situation: 
         1. 2 living neighbours & currently dead = stay dead
         2. More than 3 living neighbours = living cells die and dead cells stay dead
         */
        return 0x00;    }
    
    return 0x00;  // By default, kill the cell  (code should never reach this point though)
}

void GoL_Seed ( void )
{
    uint16_t i, j;
    uint8_t value;
    uint8_t value2;
    
    for ( i = 0; i < 24; i++)
    {
        for ( j = 0; j < 4; j++)
        {
            currentGeneration[i][j] = 0;
            nextGeneration[i][j] = 0;
        }
    }
    for ( i = 0; i < STARTING_CELLS; i++)
    {
        value = rand() % ( VERTICAL_SIZE );
        value2 = rand() % ( HORIZONTAL_SIZE );
        currentGeneration[value2][value / 8] |= 1<<(value % 8);
        nextGeneration[value2][value / 8] |= 1<<(value % 8);
    }
    Refresh_Cells ();
}

void GoL_SeedPattern ( uint8_t patternNumber )
{
    // pattern 0 = glider
    uint16_t i, j;
    
    for ( i = 0; i < 24; i++)
    {
        for ( j = 0; j < 4; j++)
        {
            currentGeneration[i][j] = 0;
            nextGeneration[i][j] = 0;
        }
    }
    
    switch (patternNumber)
    {
        case 0:
            currentGeneration[10][2] = 0b00100000;
            currentGeneration[11][2] = 0b00010000;
            currentGeneration[12][2] = 0b01110000;
            break;
        case 1:
            currentGeneration[10][2] = 0b01110000;
            currentGeneration[11][2] = 0b01010000;
            currentGeneration[12][2] = 0b01110000;
            break;
        case 2:
            currentGeneration[10][2] = 0b00100000;
            currentGeneration[11][2] = 0b01010000;
            currentGeneration[12][2] = 0b00100000;
            break;
        case 3:
            currentGeneration[23][2] = 0b11100000;
            break;
        case 4:
            currentGeneration[10][3] = 0b10000001;
            currentGeneration[11][3] = 0b10000001;
            currentGeneration[12][3] = 0b10000001;
            break;
            
        default:
            break;
    }
    Refresh_Cells ();
}

void GoL_NextGeneration ( void )
{
    uint8_t x, y;
    for ( x = 0; x < HORIZONTAL_SIZE; x++ )
    {
        for ( y = 0; y < VERTICAL_SIZE; y++ )
        {
            if ( WillCellLive ( x, y ) )
            {
                nextGeneration[x][y / 8] |= 1<<(y % 8);
            }
            else
            {
                nextGeneration[x][y / 8] &= ~(1<<(y % 8));
            }
        }
    }
    // Copy next generation to current 
    /*for ( x = 0; x < HORIZONTAL_SIZE; x++ )
    {
        for ( y = 0; y < 4; y++ )
        {
            currentGeneration[x][ y ] = nextGeneration [x][y];
            nextGeneration[x][y] = 0;
        }
    }*/
    Refresh_Cells ();
}


int main(void)
{
    SPI_Init();
    Display_Init();
    Display_Clear ( BACKGROUND_COLOR );
    //GoL_SeedPattern (0);
    GoL_Seed();
    for(;;)
    {
        _delay_ms(500);
        GoL_NextGeneration ();
    }

    return 0;
}
