#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <SDL2/SDL.h> 
#include <iostream>
using namespace std;
#define PIXEL_ON 50
#define PIXEL_OFF 0

bool isRunning;
unsigned char ramMemory[4096];
unsigned char screenBuffer[32][64]; // possible way to improve
unsigned int pc;
unsigned int IReg;
unsigned char registers[16]; // registers[0] is v0, registers[15] is vF
unsigned short stack[200];
unsigned short* stackBP = &stack[0];
unsigned short stackSP = 0;
unsigned char currentKey = 16;
unsigned char delayTimer = 0;
int fontSprites[5*16] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};
void draw(SDL_Surface* winSurface);
void handleInput(SDL_Event event);
void executeInstruction(unsigned short currentIntruction,SDL_Surface* winSurface){
    if(currentIntruction == 0x00E0){// clear
        printf("clear");
        for(int i=0;i<32;i++){
            for(int j=0;j<64;j++){
           screenBuffer[i][j] = PIXEL_OFF;
            }   
        }
    }else if(currentIntruction >> 12 == 1){ // jmp
        pc = currentIntruction & 0x0FFF;
        printf("jump");
    }else if(currentIntruction >> 12 == 0xB){ // jmp with offset posssibly ambigous
        pc = (currentIntruction & 0x0FFF) + registers[0];
        printf("jump with offset");
    }else if(currentIntruction >> 12 == 2){ // call stack
        stack[stackSP] = pc;
        stackSP +=1;
        pc = currentIntruction & 0x0FFF;
        unsigned short testeeee = currentIntruction & 0x0FFF;
        printf("call stack");
    }else if(currentIntruction == 0x00EE){ // return stack
        stackSP -=1;
        pc = stack[stackSP];
        printf("return stack");
    }else if(currentIntruction >> 12 == 0xC){ // random
        registers[(currentIntruction & 0x0F00) >> 8] = ((rand() % ((currentIntruction & 0x00FF)+1))) & (currentIntruction & 0x00FF); // possible overflow
        printf("random");
    }else if(currentIntruction >> 12 == 8){ // logical arithmetic instructions
        if((currentIntruction & 0x000F) == 0){ // set
            registers[(currentIntruction & 0x0F00) >> 8] = registers[(currentIntruction & 0x00F0) >> 4];
        }else if((currentIntruction & 0x000F) == 1){ // or
            registers[(currentIntruction & 0x0F00) >> 8] = registers[(currentIntruction & 0x0F00) >> 8] | registers[(currentIntruction & 0x00F0) >> 4];
        }else if((currentIntruction & 0x000F) == 2){ // and
            registers[(currentIntruction & 0x0F00) >> 8] = registers[(currentIntruction & 0x0F00) >> 8] & registers[(currentIntruction & 0x00F0) >> 4];
        }else if((currentIntruction & 0x000F) == 3){ // xor
            registers[(currentIntruction & 0x0F00) >> 8] = registers[(currentIntruction & 0x0F00) >> 8] ^ registers[(currentIntruction & 0x00F0) >> 4];
        }else if((currentIntruction & 0x000F) == 4){ // add
            registers[(currentIntruction & 0x0F00) >> 8] = registers[(currentIntruction & 0x0F00) >> 8] + registers[(currentIntruction & 0x00F0) >> 4];
            if(registers[(currentIntruction & 0x0F00) >> 8] + registers[(currentIntruction & 0x00F0) >> 4] > 255){
                registers[15] = 1;
            }else{
                registers[15] = 0;
            }
        }else if((currentIntruction & 0x000F) == 5){ // sub
            registers[(currentIntruction & 0x0F00) >> 8] = registers[(currentIntruction & 0x0F00) >> 8] - registers[(currentIntruction & 0x00F0) >> 4];
            if(registers[(currentIntruction & 0x0F00) >> 8] > registers[(currentIntruction & 0x00F0) >> 4]){
                registers[15] = 1;
            }else{
                registers[15] = 0;
            }
        }else if((currentIntruction & 0x000F) == 7){ // sub 2
            registers[(currentIntruction & 0x0F00) >> 8] = registers[(currentIntruction & 0x00F0) >> 4] - registers[(currentIntruction & 0x0F00) >> 8];
            if(registers[(currentIntruction & 0x0F00) >> 8] < registers[(currentIntruction & 0x00F0) >> 4]){
                registers[15] = 1;
            }else{
                registers[15] = 0;
            }
        }else if((currentIntruction & 0x000F) == 0xE){ // shift left posssibly ambigous
            registers[(currentIntruction & 0x0F00) >> 8] = registers[(currentIntruction & 0x00F0) >> 4];
            if((registers[(currentIntruction & 0x0F00) >> 8] & 0b10000000) == 1){
                registers[15] = 1;
            }else{
                registers[15] = 0;
            }
            registers[(currentIntruction & 0x0F00) >> 8] = registers[(currentIntruction & 0x0F00) >> 8] << 1;
        }else if((currentIntruction & 0x000F) == 0x6){ // shift right posssibly ambigous
            registers[(currentIntruction & 0x0F00) >> 8] = registers[(currentIntruction & 0x00F0) >> 4];
            if((registers[(currentIntruction & 0x0F00) >> 8] & 0b00000001) == 1){
                registers[15] = 1;
            }else{
                registers[15] = 0;
            }
            registers[(currentIntruction & 0x0F00) >> 8] = registers[(currentIntruction & 0x0F00) >> 8] >> 1;
        }
    }else if(currentIntruction >> 12 == 6){ // set vx
        printf("set vx");
        registers[(currentIntruction & 0x0F00)>>8] = (unsigned char) (currentIntruction & 0x00FF);
    }else if(currentIntruction >> 12 == 3){ // Skip conditionally
        printf("Skip conditionally");
        if(registers[(currentIntruction & 0x0F00) >> 8] == (currentIntruction & 0x00FF)){
            pc +=2;
        }
    }else if(currentIntruction >> 12 == 4){ // Skip conditionally 2
        printf("Skip conditionally");
        if(registers[(currentIntruction & 0x0F00) >> 8] != (currentIntruction & 0x00FF)){
            pc +=2;
        }
    }else if(currentIntruction >> 12 == 5){ // Skip conditionally 3
        printf("Skip conditionally");
        if(registers[(currentIntruction & 0x0F00) >> 8] == (registers[(currentIntruction & 0x00F0) >> 4])){
            pc +=2;
        }
    }else if(currentIntruction >> 12 == 9){ // Skip conditionally 4
        printf("Skip conditionally");
        if(registers[(currentIntruction & 0x0F00) >> 8] != (registers[(currentIntruction & 0x00F0) >> 4])){
            pc +=2;
        }
    }else if(currentIntruction >> 12 == 7){ // add vx
        printf("add vx");
        registers[(currentIntruction & 0x0F00)>>8] += (unsigned char) (currentIntruction & 0x00FF);
    }else if(((currentIntruction >> 12) == 0xF) && ((currentIntruction & 0x00FF) == 0x1E)){ // add index R
        printf("add inedx");
        IReg += registers[currentIntruction & 0x0F00];
    }else if(((currentIntruction >> 12) == 0xF) && ((currentIntruction & 0x00FF) == 0x29)){ // set index R to font
        printf("set index R to font");
        IReg =0x50 + registers[(currentIntruction & 0x0F00)>>8] * 5; // idk
    }else if(((currentIntruction >> 12)== 0xF) && ((currentIntruction & 0x00FF) == 0x0A)){ // get key
        if(currentKey == 16){
            pc -= 2;
        }else{
            registers[(currentIntruction & 0x0F00)>>8] = currentKey;
        }
    }else if(((currentIntruction >> 12) == 0xE) && ((currentIntruction & 0x00FF) == 0x9E)){ // advance if key
        printf("advance if key");
        if(registers[(currentIntruction & 0x0F00)>>8] == currentKey){
            pc += 2;
        }
    }else if(((currentIntruction >> 12) == 0xE) && ((currentIntruction & 0x00FF) == 0xA1)){ // advance if not key
        printf("advance if not key");
        if(registers[(currentIntruction & 0x0F00)>>8] != currentKey){
            pc += 2;
        }
    }else if(((currentIntruction >> 12) == 0xF) && ((currentIntruction & 0x00FF) == 0x07)){ // set vx to timer
        printf("set vx to timer");
        registers[(currentIntruction & 0x0F00)>>8] = delayTimer;
    }else if((((currentIntruction >> 12) == 0xF) && ((currentIntruction & 0x00FF) == 0x15))  ){ // set timer to vx
        printf("set timer to vx");
        delayTimer = registers[(currentIntruction & 0x0F00)>>8];
        SDL_Delay(delayTimer*17);
    }else if(((currentIntruction >> 12) == 0xF) && ((currentIntruction & 0x00FF) == 0x33)){ // Binary-coded decimal conversion
        printf("Binary-coded decimal conversion");
        float binX = currentIntruction & 0x0F00 >> 8;
        char cent = binX/100;
        char dec = (binX - cent*100)/10;
        char uni = binX - cent*100-dec*10;  // can get errors
        ramMemory[IReg] = cent;
        ramMemory[IReg+1] = dec;
        ramMemory[IReg+2] = uni;
    }else if(((currentIntruction >> 12) == 0xF) && ((currentIntruction & 0x00FF) == 0x55)){ // save to memory
        printf("save to memory");
        for(int i=0;i<= (currentIntruction & 0x0F00 >> 8);i++){
            ramMemory[IReg+i] = registers[i];
        }
    }else if(((currentIntruction >> 12) == 0xF) && ((currentIntruction & 0x00FF) == 0x65)){ // load from memory
        printf("load from memory");
        for(int i=0;i<= (currentIntruction & 0x0F00 >> 8);i++){
            registers[i] = ramMemory[IReg+i];
        }
    }else if(currentIntruction >> 12 == 0xA){ // set index R
        printf("set inedx");
        IReg = currentIntruction & 0x0FFF;
    }else if(currentIntruction >> 12 == 0xD){ // draw
        printf("draw");
        char x = (registers[(currentIntruction & 0x0F00)>>8] % 63);
        char y = (registers[(currentIntruction & 0x00F0)>>4] % 31);
        int N = currentIntruction & 0x000F;
        registers[15] = 0;
        printf("x%x",x);
        printf("y%x",y);
        printf("N%x",N);
        for(int i = 0;i<N;i++){
            for(int j=0;j<8;j++){
                if(y+i< 32 && x+j < 64){
                    if((screenBuffer[y+i][x+j] == PIXEL_OFF) && (((ramMemory[IReg+i] >> (7-j)) & 0b00000001) == 0b00000001) ){
                        screenBuffer[y+i][x+j] = PIXEL_ON;
                        printf("foi ");
                    }
                    else if((screenBuffer[y+i][x+j] == PIXEL_ON) && (((ramMemory[IReg+i] >> (7-j)) & 0b00000001) == 0b00000001) ){
                        screenBuffer[y+i][x+j] = PIXEL_OFF;
                        registers[15] = 1;
                        printf("foinao ");
                    }
                }
            }
        }
    draw(winSurface);
    }else if(currentIntruction == 1){ //

    }else{
        printf("instruction doesnt exist");
    }
}

int main(int argc, char** args){
    unsigned short currentInstruction;
    pc = 0;

    //Load Rom to ram and put on adress 0x200 512
    FILE* file;
    file = fopen("Airplane.ch8", "rb");
    unsigned char Temp[2000];
    fread(Temp, 1, sizeof(Temp), file);
    if (file == NULL) {
    perror("Failed to open file");
    return 1;
}
    fclose(file);

    for(int i=0;i<=0x9F-0x50;i++){
        ramMemory[i+0x50] = fontSprites[i]; 
    }

    for(int i=0;i<2000;i++){
        ramMemory[i+512] = Temp[i]; // Possible for Temp values to be non zero above 100
    }
    pc = 512;


    // Initialize SDL
    SDL_Surface* winSurface = NULL;
	SDL_Window* window = NULL;

	if ( SDL_Init( SDL_INIT_EVERYTHING ) < 0 ) {
		cout << "Error initializing SDL: " << SDL_GetError() << endl;
		system("pause");
		return 1;
	} 

	window = SDL_CreateWindow( "Example", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 720, SDL_WINDOW_SHOWN );

	if ( !window ) {
		cout << "Error creating window: " << SDL_GetError()  << endl;
		system("pause");
		return 1;
	}

	winSurface = SDL_GetWindowSurface( window );

	if ( !winSurface ) {
		cout << "Error getting surface: " << SDL_GetError() << endl;
		system("pause");
		return 1;
	}
	SDL_FillRect( winSurface, NULL, SDL_MapRGB( winSurface->format, 255, 255, 255 ) );
	SDL_UpdateWindowSurface( window );
    
    
    
    
    // Fetch Decode Execute cycle
    srand(time(NULL));
    SDL_Rect *rect1 = new SDL_Rect();
    rect1->w = 10;
    rect1->h = 10;
    rect1->x = 10;
    rect1->y = 10;
    Uint32 currentTime;
    Uint32 lastTime;
    Uint32 deltaTime;
    SDL_Event event;
    int countToTen = 0;
    isRunning = true;
    while(isRunning){
        if(countToTen == 10){
            countToTen = 0;
            if(delayTimer > 0){
                delayTimer -=1;
            }
        }
        lastTime = SDL_GetTicks();

        while (SDL_PollEvent(&event))
        {
            handleInput(event);
        }

        currentInstruction = 0;
        currentInstruction = ((unsigned short) ramMemory[pc]) * 256 + ((unsigned short) ramMemory[pc+1]);
        //printf("\npc%i",pc);
        //printf("\ncurrentIntruction%x",currentInstruction);
        pc += 2;
        executeInstruction(currentInstruction,winSurface);
        SDL_UpdateWindowSurface( window );

        // Avoid negative delays
        currentTime = SDL_GetTicks();
        if (currentTime - lastTime > 2)
        {
            deltaTime = 0;
        }
        else
        {
            deltaTime = currentTime - lastTime;
        }

        // Delay function to have max 60 fps
        SDL_Delay(2 - deltaTime);
        countToTen +=1;
    }
    /*
    for(int i=0;i<=200;i++){
    }
    */
	SDL_DestroyWindow( window );
	SDL_Quit();
	return 0;
}

void draw(SDL_Surface* winSurface){
    SDL_FillRect(winSurface, NULL, SDL_MapRGB(winSurface->format, 8, 23, 43));
    SDL_Rect *rect1 = new SDL_Rect();
    rect1->w = 10;
    rect1->h = 10;
    for (int i = 0; i < 32; i++){
        for (int j = 0; j < 64; j++){
            if (screenBuffer[i][j] == PIXEL_ON){
                rect1->y = 10*i;
                rect1->x = 10*j;
                SDL_FillRect(winSurface, rect1, SDL_MapRGB(winSurface->format, 230, 187, 131));
                
            }
        }
    }
}

void handleInput(SDL_Event event){
    switch (event.type)
    {
    case SDL_QUIT:
        isRunning = false;
        break;
    case SDL_KEYDOWN:
        if (event.key.keysym.sym == SDLK_1)
        {
            currentKey = 0x0;
            break;
        }
        else if (event.key.keysym.sym == SDLK_2)
        {
            currentKey = 0x1;
            break;
        }
        else if (event.key.keysym.sym == SDLK_3)
        {
            currentKey = 0x2;
            break;
        }
        else if (event.key.keysym.sym == SDLK_4)
        {
            currentKey = 0x3;
            break;
        }
        else if (event.key.keysym.sym == SDLK_q)
        {
            currentKey = 0x4;
            break;
        }
        else if (event.key.keysym.sym == SDLK_w)
        {
            currentKey = 0x5;
            break;
        }
        else if (event.key.keysym.sym == SDLK_e)
        {
            currentKey = 0x6;
            break;
        }
        else if (event.key.keysym.sym == SDLK_r)
        {
            currentKey = 0x7;
            break;
        }
        else if (event.key.keysym.sym == SDLK_a)
        {
            currentKey = 0x8;
            break;
        }
        else if (event.key.keysym.sym == SDLK_s)
        {
            currentKey = 0x9;
            break;
        }
        else if (event.key.keysym.sym == SDLK_d)
        {
            currentKey = 0xA;
            break;
        }
        else if (event.key.keysym.sym == SDLK_f)
        {
            currentKey = 0xB;
            break;
        }
        else if (event.key.keysym.sym == SDLK_z)
        {
            currentKey = 0xC;
            break;
        }
        else if (event.key.keysym.sym == SDLK_x)
        {
            currentKey = 0xD;
            break;
        }
        else if (event.key.keysym.sym == SDLK_c)
        {
            currentKey = 0xE;
            break;
        }
        else if (event.key.keysym.sym == SDLK_v)
        {
            currentKey = 0xF;
            break;
        }

     case SDL_KEYUP:
        if (event.key.keysym.sym == SDLK_1)
        {
            currentKey = 0x10;
            break;
        }
        else if (event.key.keysym.sym == SDLK_2)
        {
            currentKey = 0x10;
            break;
        }
        else if (event.key.keysym.sym == SDLK_3)
        {
            currentKey = 0x10;
            break;
        }
        else if (event.key.keysym.sym == SDLK_4)
        {
            currentKey = 0x10;
            break;
        }
        else if (event.key.keysym.sym == SDLK_q)
        {
            currentKey = 0x10;
            break;
        }
        else if (event.key.keysym.sym == SDLK_w)
        {
            currentKey = 0x10;
            break;
        }
        else if (event.key.keysym.sym == SDLK_e)
        {
            currentKey = 0x10;
            break;
        }
        else if (event.key.keysym.sym == SDLK_r)
        {
            currentKey = 0x10;
            break;
        }
        else if (event.key.keysym.sym == SDLK_a)
        {
            currentKey = 0x10;
            break;
        }
        else if (event.key.keysym.sym == SDLK_s)
        {
            currentKey = 0x10;
            break;
        }
        else if (event.key.keysym.sym == SDLK_d)
        {
            currentKey = 0x10;
            break;
        }
        else if (event.key.keysym.sym == SDLK_f)
        {
            currentKey = 0x10;
            break;
        }
        else if (event.key.keysym.sym == SDLK_z)
        {
            currentKey = 0x10;
            break;
        }
        else if (event.key.keysym.sym == SDLK_x)
        {
            currentKey = 0x10;
            break;
        }
        else if (event.key.keysym.sym == SDLK_c)
        {
            currentKey = 0x10;
            break;
        }
        else if (event.key.keysym.sym == SDLK_v)
        {
            currentKey = 0x10;
            break;
        }   
    }
}