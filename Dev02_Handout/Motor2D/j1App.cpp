#include <iostream> 
#include <sstream> 

#include "p2Defs.h"
#include "p2Log.h"

#include "j1Window.h"
#include "j1Input.h"
#include "j1Render.h"
#include "j1Textures.h"
#include "j1Audio.h"
#include "j1Scene.h"
#include "j1App.h"

// Constructor
j1App::j1App(int argc, char* args[]) : argc(argc), args(args)
{
	frames = 0;

	input = new j1Input();
	win = new j1Window();
	render = new j1Render();
	tex = new j1Textures();
	audio = new j1Audio();
	scene = new j1Scene();

	// Ordered for awake / Start / Update
	// Reverse order of CleanUp
	AddModule(input);
	AddModule(win);
	AddModule(tex);
	AddModule(audio);
	AddModule(scene);

	// render last to swap buffer
	AddModule(render);
}

// Destructor
j1App::~j1App()
{
	// release modules
	p2List_item<j1Module*>* item = modules.end;

	while(item != NULL)
	{
		RELEASE(item->data);
		item = item->prev;
	}

	modules.clear();

	config_file.reset();
}

void j1App::AddModule(j1Module* module)
{
	module->Init();
	modules.add(module);
}

// Called before render is available
bool j1App::Awake()
{
	bool ret = LoadConfig();
	// self-config
	title.create(app_config.child("title").child_value());
	organization.create(app_config.child("organization").child_value());

	if(ret == true)
	{
		p2List_item<j1Module*>* item;
		item = modules.start;

		while(item != NULL && ret == true)
		{
			ret = item->data->Awake(config.child(item->data->name.GetString()));
			item = item->next;
		}
	}

	return ret;
}

// Called before the first frame
bool j1App::Start()
{
	bool ret = true;
	p2List_item<j1Module*>* item;
	item = modules.start;

	while(item != NULL && ret == true)
	{
		ret = item->data->Start();
		item = item->next;
	}

	return ret;
}

// Called each loop iteration
bool j1App::Update()
{
	bool ret = true;
	PrepareUpdate();

	if(input->GetWindowEvent(WE_QUIT) == true)
		ret = false;

	if(ret == true)
		ret = PreUpdate();

	if(ret == true)
		ret = DoUpdate();

	if(ret == true)
		ret = PostUpdate();

	FinishUpdate();
	return ret;
}


// ---------------------------------------------
bool j1App::LoadConfig()
{
	bool ret = true;

	pugi::xml_parse_result result = config_file.load_file("config.xml");
	savegame_file.load_file("savegame.xml");

	if(result == NULL)
	{
		LOG("Could not load map xml file config.xml. pugi error: %s", result.description());
		ret = false;
	}
	else
	{
		config = config_file.child("config");
		app_config = config.child("app");
	}

	return ret;
}



// ---------------------------------------------
void j1App::PrepareUpdate()
{
}

// ---------------------------------------------
void j1App::FinishUpdate()
{
	if (save_request)
	{
		Save();
		save_request = false;
	}
	if (load_request)
	{
		Load();
		load_request = false;
	}

	if (volumeup)
	{
		App->audio->Musicup();
		volumeup = false;
	}

	if (volumedown)
	{
		App->audio->Musicdown();
		volumedown = false;
	}

	// TODO 2: This is a good place to call load / Save functions

}

// Call modules before each loop iteration
bool j1App::PreUpdate()
{
	bool ret = true;
	p2List_item<j1Module*>* item;
	item = modules.start;
	j1Module* pModule = NULL;

	for(item = modules.start; item != NULL && ret == true; item = item->next)
	{
		pModule = item->data;

		if(pModule->active == false) {
			continue;
		}

		ret = item->data->PreUpdate();
	}

	return ret;
}

// Call modules on each loop iteration
bool j1App::DoUpdate()
{
	bool ret = true;
	p2List_item<j1Module*>* item;
	item = modules.start;
	j1Module* pModule = NULL;

	for(item = modules.start; item != NULL && ret == true; item = item->next)
	{
		pModule = item->data;

		if(pModule->active == false) {
			continue;
		}

		ret = item->data->Update(dt);
	}
	return ret;
}

// Call modules after each loop iteration
bool j1App::PostUpdate()
{
	bool ret = true;
	p2List_item<j1Module*>* item;
	j1Module* pModule = NULL;

	for(item = modules.start; item != NULL && ret == true; item = item->next)
	{
		pModule = item->data;

		if(pModule->active == false) {
			continue;
		}
		ret = item->data->PostUpdate();
	}

	return ret;
}

// Called before quitting
bool j1App::CleanUp()
{
	bool ret = true;
	p2List_item<j1Module*>* item;
	item = modules.end;

	while(item != NULL && ret == true)
	{
		ret = item->data->CleanUp();
		item = item->prev;
	}

	return ret;
}

// ---------------------------------------
int j1App::GetArgc() const
{
	return argc;
}

// ---------------------------------------
const char* j1App::GetArgv(int index) const
{
	if(index < argc)
		return args[index];
	else
		return NULL;
}

// ---------------------------------------
const char* j1App::GetTitle() const
{
	return title.GetString();
}

// ---------------------------------------
const char* j1App::GetOrganization() const
{
	return organization.GetString();
}


void j1App::Save()
{
		p2List_item<j1Module*>* item;
		item = modules.start;

		pugi::xml_document data;
		pugi::xml_node save_node= data.append_child("save");

		while (item != NULL)
		{
			item->data->Save(save_node.append_child(item->data->name.GetString()));
			item = item->next;
		}
			data.save_file("savegame.xml");
}

void j1App::Load()
{
	pugi::xml_parse_result result = savegame_file.load_file("savegame.xml");
		//LOG("Could not load map xml file savegame.xml. pugi error: %s", result.description());
	
	savegame = savegame_file.child("save");
	p2List_item<j1Module*>* item;
	item = modules.start;

	while (item != NULL)
	{
		item->data->Load(savegame.child(item->data->name.GetString()));
		item = item->next;
	}
}



// TODO 4: Create a simulation of the xml file to read 

// TODO 5: Create a method to actually load an xml file
// then call all the modules to load themselves

// TODO 7: Create a method to save the current state

