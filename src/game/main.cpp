#include <iostream>

#include "prism/Prism.h"
#include "Voxel.h"
#include "prism/System/DynamicArray.h"

int main()
{
	Prism::Application app(1280, 720, "Prism");
	
	app.CreateLayer<WorldGen>("Voxel Example");
	
	app.Run();
	
	return 0;
}
