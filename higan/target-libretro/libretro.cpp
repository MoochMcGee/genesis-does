#include "libretro.h"
#include <nall/nall.hpp>
#include <emulator/emulator.hpp>

#include <fc/interface/interface.hpp>
#include <sfc/interface/interface.hpp>
#include <ms/interface/interface.hpp>
#include <md/interface/interface.hpp>
#include <pce/interface/interface.hpp>
#include <gb/interface/interface.hpp>
#include <gba/interface/interface.hpp>
#include <ws/interface/interface.hpp>

static string locate(string name)
{
	string location = { Path::config(), "higan/", name };
	if (inode::exists(location))
		return location;

	directory::create({ Path::local(), "higan/" });
	return { Path::local(), "higan/", name };
}

// Bake in icarus, so we can build BML manifests.
#include "../../icarus/settings.cpp"
static Settings settings;

#include "../../icarus/heuristics/famicom.cpp"
#include "../../icarus/heuristics/super-famicom.cpp"
#include "../../icarus/heuristics/master-system.cpp"
#include "../../icarus/heuristics/mega-drive.cpp"
#include "../../icarus/heuristics/pc-engine.cpp"
#include "../../icarus/heuristics/supergrafx.cpp"
#include "../../icarus/heuristics/game-boy.cpp"
#include "../../icarus/heuristics/game-boy-advance.cpp"
#include "../../icarus/heuristics/game-gear.cpp"
#include "../../icarus/heuristics/wonderswan.cpp"
#include "../../icarus/heuristics/bs-memory.cpp"
#include "../../icarus/heuristics/sufami-turbo.cpp"

#include "../../icarus/core/core.hpp"
#include "../../icarus/core/core.cpp"
#include "../../icarus/core/famicom.cpp"
#include "../../icarus/core/super-famicom.cpp"
#include "../../icarus/core/master-system.cpp"
#include "../../icarus/core/mega-drive.cpp"
#include "../../icarus/core/pc-engine.cpp"
#include "../../icarus/core/supergrafx.cpp"
#include "../../icarus/core/game-boy.cpp"
#include "../../icarus/core/game-boy-color.cpp"
#include "../../icarus/core/game-boy-advance.cpp"
#include "../../icarus/core/game-gear.cpp"
#include "../../icarus/core/wonderswan.cpp"
#include "../../icarus/core/wonderswan-color.cpp"
#include "../../icarus/core/bs-memory.cpp"
#include "../../icarus/core/sufami-turbo.cpp"
static Icarus icarus;

static retro_environment_t environ_cb;
static retro_video_refresh_t video_cb;
static retro_audio_sample_t audio_cb;
static retro_input_poll_t input_poll;
static retro_input_state_t input_state;

struct Program : Emulator::Platform
{
	Program();
	~Program();

	vector<Emulator::Interface *> emulators;
	Emulator::Interface *emulator = nullptr;

	string path(uint id) override;
	vfs::shared::file open(uint id, string name, vfs::file::mode mode, bool required) override;
	Emulator::Platform::Load load(uint id, string name, string type, string_vector options) override;
	void videoRefresh(const uint32 *data, uint pitch, uint width, uint height) override;
	void audioSample(const double *samples, uint channels) override;
	int16 inputPoll(uint port, uint device, uint input) override;
	void inputRumble(uint port, uint device, uint input, bool enable) override;
	uint dipSettings(Markup::Node node) override;
	void notify(string text) override;

	vector<string> medium_paths;
	vector<vfs::shared::file> vfs_files;
	vfs::shared::file rom_file;
	uint game_system_id = ~0u;
};

static Program *program = nullptr;

Program::Program()
{
	Emulator::platform = this;

	emulators.append(new Famicom::Interface);
	emulators.append(new SuperFamicom::Interface);
	emulators.append(new MasterSystem::MasterSystemInterface);
	emulators.append(new MegaDrive::Interface);
	emulators.append(new PCEngine::PCEngineInterface);
	emulators.append(new PCEngine::SuperGrafxInterface);
	emulators.append(new GameBoy::GameBoyInterface);
	emulators.append(new GameBoy::GameBoyColorInterface);
	emulators.append(new GameBoyAdvance::Interface);
	emulators.append(new MasterSystem::GameGearInterface);
	emulators.append(new WonderSwan::WonderSwanInterface);
	emulators.append(new WonderSwan::WonderSwanColorInterface);
}

Program::~Program()
{
	for (auto &emulator : emulators)
		delete emulator;
	emulators.reset();
}

string Program::path(uint id)
{
	return medium_paths(id);
}

vfs::shared::file Program::open(uint id, string name, vfs::file::mode mode, bool required)
{
	// TODO: Generate this inside the process by baking in icarus.
	if (name == "manifest.bml" && !path(id).endsWith(".sys/"))
	{
		string imported_path = icarus.import(path(id));
		medium_paths(id) = imported_path;
		string manifest = icarus.manifest(imported_path);
		return vfs::memory::file::open(manifest.data<uint8_t>(), manifest.size());
	}

	// Icarus::import currently forces files to disk, so the only thing we can do
	// here is to obey for now ...
	if (auto result = vfs::fs::file::open({ path(id), name }, mode))
		return result;

	//return vfs_files(id);
	return {};
}

Emulator::Platform::Load Program::load(uint id, string name, string, string_vector)
{
	return { id, name };
}

void Program::videoRefresh(const uint32 *data, uint pitch, uint width, uint height)
{
	video_cb(data, width, height, pitch);
}

static int16_t d2i16(double v)
{
	v *= 0x8000;
	if (v > 0x7fff)
		v = 0x7fff;
	else if (v < -0x8000)
		v = -0x8000;
	return int16_t(v);
}

void Program::audioSample(const double *samples, uint channels)
{
	int16_t left = d2i16(samples[0]);
	int16_t right = d2i16(samples[1]);
	audio_cb(left, right);
}

int16 Program::inputPoll(uint port, uint device, uint input)
{
}

void Program::inputRumble(uint port, uint device, uint input, bool enable)
{
}

uint Program::dipSettings(Markup::Node node)
{
}

void Program::notify(string text)
{
}

RETRO_API void retro_set_environment(retro_environment_t cb)
{
	environ_cb = cb;
}

RETRO_API void retro_set_video_refresh(retro_video_refresh_t cb)
{
	video_cb = cb;
}

RETRO_API void retro_set_audio_sample(retro_audio_sample_t cb)
{
	audio_cb = cb;
}

RETRO_API void retro_set_audio_sample_batch(retro_audio_sample_batch_t)
{
}

RETRO_API void retro_set_input_poll(retro_input_poll_t cb)
{
	input_poll = cb;
}

RETRO_API void retro_set_input_state(retro_input_state_t cb)
{
	input_state = cb;
}

RETRO_API void retro_init()
{
	program = new Program;
}

RETRO_API void retro_deinit()
{
	delete program;
	program = nullptr;
}

RETRO_API unsigned retro_api_version()
{
	return RETRO_API_VERSION;
}

RETRO_API void retro_get_system_info(retro_system_info *info)
{
	info->library_name = "higan";
	info->library_version = Emulator::Version;
	info->valid_extensions = "sfc";
	info->need_fullpath = true;
	info->block_extract = false;
}

RETRO_API void retro_get_system_av_info(struct retro_system_av_info *info)
{
	info->geometry.base_width = 256;
	info->geometry.base_height = 240;
	info->geometry.max_width = 512;
	info->geometry.max_height = 480;
	info->geometry.aspect_ratio = 4.0f / 3.0f;
	info->timing.fps = 60.0f;
	info->timing.sample_rate = 44100.0f;
}

RETRO_API void retro_set_controller_port_device(unsigned port, unsigned device)
{
}

RETRO_API void retro_reset()
{
	program->emulator->power();
}

RETRO_API void retro_run()
{
	input_poll();
	program->emulator->run();
}

RETRO_API size_t retro_serialize_size()
{
	return 0;
}

RETRO_API bool retro_serialize(void *data, size_t size)
{
	return false;
}

RETRO_API bool retro_unserialize(const void *data, size_t size)
{
	return false;
}

RETRO_API void retro_cheat_reset()
{
}

RETRO_API void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
}

RETRO_API bool retro_load_game(const retro_game_info *game)
{
	if (program->emulator)
		return false;

	retro_pixel_format fmt = RETRO_PIXEL_FORMAT_XRGB8888;
	if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
		return false;

	Emulator::audio.reset(2, 44100);
	string location = game->path;
	string type = Location::suffix(location).trimLeft(".", 1L);
	const Emulator::Interface::Medium *emulator_medium = nullptr;

	for (auto &emulator : program->emulators)
	{
		for (auto &medium : emulator->media)
		{
			if (medium.type == type)
			{
				program->emulator = emulator;
				emulator_medium = &medium;
				goto out;
			}
		}
	}
out:

	// ID 0 is the system path.
	// This should probably be either baked into the core, or be redirected through environment callbacks.
	// Asset directory perhaps?
	program->medium_paths.append(locate({ emulator_medium->name, ".sys/" }));

	//program->rom_file = vfs::memory::file::open(static_cast<const uint8_t *>(game->data), game->size);
	program->medium_paths(emulator_medium->id) = location;
	//program->vfs_files(emulator_medium->id) = program->rom_file;
	//program->game_system_id = emulator_medium->id;

	if (!program->emulator || !emulator_medium)
		return false;

	if (!program->emulator->load(emulator_medium->id))
		return false;

	if (!program->emulator->loaded())
		return false;

	program->emulator->power();
	Emulator::video.setSaturation(1.0);
	Emulator::video.setGamma(2.2);
	Emulator::video.setLuminance(1.0);
	Emulator::video.setPalette();
	Emulator::audio.setFrequency(44100.0);
	Emulator::audio.setVolume(1.0);
	Emulator::audio.setBalance(0.0);
	Emulator::audio.setReverb(false);

	for (auto &port : program->emulator->ports)
	{
		auto path = string{program->emulator->information.name, "/", port.name}.replace(" ", "");
		auto name = settings(path).text();
		for (auto &device : port.devices)
		{
			if (device.name == name)
			{
				program->emulator->connect(port.id, device.id);
				break;
			}
		}
	}

	return true;
}

RETRO_API bool retro_load_game_special(unsigned game_type,
		const struct retro_game_info *info, size_t num_info)
{
	return false;
}

RETRO_API void retro_unload_game()
{
	program->emulator->unload();
}

RETRO_API unsigned retro_get_region()
{
}

RETRO_API void *retro_get_memory_data(unsigned id)
{
	return nullptr;
}

RETRO_API size_t retro_get_memory_size(unsigned id)
{
	return 0;
}
