/*
 * tested on ubuntu 22.04.1 LTS
 */

#include "../../rx/rx.h"
#include <stdio.h>
#include <string.h>
#include <malloc.h>

// keys: 107 = mouse1, 108 = mouse2, 109 = mouse3, 110 = mouse4, 111 = mouse5
#define AIMKEY 111
#define AIMFOV 5.0f
#define AIMSMOOTH 20.0f
#define GLOW_ESP 1

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned int DWORD;
typedef unsigned long QWORD;
typedef int BOOL;
typedef const char *PCSTR;
typedef const unsigned short *PCWSTR;
typedef void *PVOID;

typedef struct
{
	float x, y, z;
} vec3;

float qsqrt(float x)
{
	union
	{
		int i;
		float f;
	} u;

	u.f = x;
	u.i = (1 << 29) + (u.i >> 1) - (1 << 22);

	u.f = u.f + x / u.f;
	u.f = 0.25f * u.f + x / u.f;

	return u.f;
}

float vec_length_sqrt(vec3 p0)
{
	return (float)qsqrt(p0.x * p0.x + p0.y * p0.y + p0.z * p0.z);
}

vec3 vec_sub(vec3 p0, vec3 p1)
{
	vec3 r;

	r.x = p0.x - p1.x;
	r.y = p0.y - p1.y;
	r.z = p0.z - p1.z;
	return r;
}

float qfloor(float x)
{
	if (x >= 0.0f)
		return (float)((int)x);
	return (float)((int)x - 1);
}

float qfmodf(float a, float b)
{
	return (a - b * qfloor(a / b));
}

void vec_clamp(vec3 *v)
{

	if (v->x > 89.0f && v->x <= 180.0f)
	{
		v->x = 89.0f;
	}
	if (v->x > 180.0f)
	{
		v->x = v->x - 360.0f;
	}
	if (v->x < -89.0f)
	{
		v->x = -89.0f;
	}
	v->y = qfmodf(v->y + 180, 360) - 180;
	v->z = 0;
}

#define X_PI 3.14159f 
#define X_DIV 57.29577f
#define qabs(x) ((x) < 0 ? -(x) : (x))
#define qmax(a, b) (((a) > (b)) ? (a) : (b))
#define qmin(a, b) (((a) < (b)) ? (a) : (b))

float qatan2(float y, float x)
{
	float t0, t1, t3, t4;
	t3 = qabs(x);
	t1 = qabs(y);
	t0 = qmax(t3, t1);
	t1 = qmin(t3, t1);
	t3 = 1 / t0;
	t3 = t1 * t3;

	t4 = t3 * t3;
	t0 = -0.0134804f;
	t0 = t0 * t4 + 0.05747f;
	t0 = t0 * t4 - 0.12123f;
	t0 = t0 * t4 + 0.19563f;
	t0 = t0 * t4 - 0.33299f;
	t0 = t0 * t4 + 0.99999f;
	t3 = t0 * t3;

	t3 = (qabs(y) > qabs(x)) ? 1.57079f - t3 : t3;
	t3 = (x < 0) ? X_PI - t3 : t3;
	t3 = (y < 0) ? -t3 : t3;

	return t3;
}
float vec_distance(vec3 p0, vec3 p1)
{
	return vec_length_sqrt(vec_sub(p0, p1));
}

float qatan(float x)
{
	return qatan2(x, 1);
}

#define M_PI 3.14159265358979323846264338327950288
vec3 CalcAngle(vec3 src, vec3 dst)
{
	vec3 angle;

	vec3 delta = vec_sub(src, dst);

	float hyp = qsqrt(delta.x * delta.x + delta.y * delta.y);

	angle.x = qatan(delta.z / hyp) * (float)(180.0 / M_PI);
	angle.y = qatan(delta.y / delta.x) * (float)(180.0 / M_PI);
	angle.z = 0;

	if (delta.x >= 0.0)
		angle.y += 180.0f;

	return angle;
}

double qpow(double a, double b) {
	double c = 1;
	for (int i = 0; i < b; i++)
		c *= a;
	return c;
}

float get_fov(vec3 scrangles, vec3 aimangles)
{
	vec3 delta;

	delta.x = aimangles.x - scrangles.x;
	delta.y = aimangles.y - scrangles.y;
	delta.z = aimangles.z - scrangles.z;

	if (delta.x > 180)
		delta.x = 360 - delta.x;
	if (delta.x < 0)
		delta.x = -delta.x;

	delta.y = qfmodf(delta.y + 180, 360) - 180;
	if (delta.y < 0)
		delta.y = -delta.y;

	return qsqrt((float)(qpow(delta.x, 2.0) + qpow(delta.y, 2.0)));
}

//
// some windows extensions for rx library
// these are required because we are working with PE images
//
QWORD rx_dump_module(rx_handle process, QWORD base);
void rx_free_module(QWORD dumped_module);
QWORD rx_scan_pattern(QWORD dumped_module, PCSTR pattern, PCSTR mask, QWORD length);
QWORD rx_read_i64(rx_handle process, QWORD address);
DWORD rx_read_i32(rx_handle process, QWORD address);
WORD rx_read_i16(rx_handle process, QWORD address);
BYTE rx_read_i8(rx_handle process, QWORD address);
float rx_read_float(rx_handle process, QWORD address);
BOOL rx_write_i32(rx_handle process, QWORD address, DWORD value);

QWORD ResolveRelativeAddressEx(
    rx_handle process,
    QWORD Instruction,
    DWORD OffsetOffset,
    DWORD InstructionSize);

int GetApexProcessId(void)
{
	int pid = 0;
	rx_handle snapshot = rx_create_snapshot(RX_SNAP_TYPE_PROCESS, 0);

	RX_PROCESS_ENTRY entry;

	while (rx_next_process(snapshot, &entry))
	{
		if (!strcmp(entry.name, "wine64-preloader"))
		{
			rx_handle snapshot_2 = rx_create_snapshot(RX_SNAP_TYPE_LIBRARY, entry.pid);

			RX_LIBRARY_ENTRY library_entry;

			while (rx_next_library(snapshot_2, &library_entry))
			{
				if (!strcmp(library_entry.name, "easyanticheat_x64.dll"))
				{
					pid = entry.pid;
					break;
				}
			}
			rx_close_handle(snapshot_2);

			//
			// process found
			//
			if (pid != 0)
			{
				break;
			}
		}
	}
	rx_close_handle(snapshot);

	return pid;
}

QWORD GetApexBaseAddress(int pid)
{
	rx_handle snapshot = rx_create_snapshot(RX_SNAP_TYPE_LIBRARY, pid);

	RX_LIBRARY_ENTRY entry;
	DWORD counter = 0;
	QWORD base = 0;

	while (rx_next_library(snapshot, &entry))
	{
		const char *sub = strstr(entry.name, "memfd:wine-mapping");

		if ((entry.end - entry.start) == 0x1000 && sub)
		{
			if (counter == 0)
				base = entry.start;
		}

		if (sub)
		{
			counter++;
		}

		else
		{
			counter = 0;
			base = 0;
		}

		if (counter >= 200)
		{
			break;
		}
	}

	rx_close_handle(snapshot);

	return base;
}

typedef struct
{
	uint8_t pad1[0xCC];
	float x;
	uint8_t pad2[0xC];
	float y;
	uint8_t pad3[0xC];
	float z;
} matrix3x4_t;

int m_iHealth;
int m_iTeamNum;
int m_iViewAngles;
int m_iCameraAngles;
int m_bZooming;
int m_iBoneMatrix;
int m_iWeapon;
int m_vecAbsOrigin;
int m_playerData;
int m_lifeState;

QWORD GetClientEntity(rx_handle game_process, QWORD entity, QWORD index)
{

	index = index + 1;
	index = index << 0x5;

	return rx_read_i64(game_process, (index + entity) - 0x280050);
}

QWORD get_interface_function(rx_handle game_process, QWORD ptr, DWORD index)
{
	return rx_read_i64(game_process, rx_read_i64(game_process, ptr) + index * 8);
}

vec3 GetBonePosition(rx_handle game_process, QWORD entity_address, int index)
{
	vec3 position;
	rx_read_process(game_process, entity_address + m_vecAbsOrigin, &position, sizeof(position));

	QWORD bonematrix = rx_read_i64(game_process, entity_address + m_iBoneMatrix);

	matrix3x4_t matrix;
	rx_read_process(game_process, bonematrix + (0x30 * index), &matrix, sizeof(matrix3x4_t));

	vec3 bonepos;
	bonepos.x = matrix.x + position.x;
	bonepos.y = matrix.y + position.y;
	bonepos.z = matrix.z + position.z;

	return bonepos;
}

BOOL IsButtonDown(rx_handle game_process, QWORD IInputSystem, int KeyCode)
{
	KeyCode = KeyCode + 1;
	DWORD a0 = rx_read_i32(game_process, IInputSystem + ((KeyCode >> 5) * 4) + 0xb0);
	return (a0 >> (KeyCode & 31)) & 1;
}

int dump_table(rx_handle game_process, QWORD table, const char *name)
{

	for (DWORD i = 0; i < rx_read_i32(game_process, table + 0x10); i++)
	{

		QWORD recv_prop = rx_read_i64(game_process, table + 0x8);
		if (!recv_prop)
		{
			continue;
		}

		recv_prop = rx_read_i64(game_process, recv_prop + 0x8 * i);
		char recv_prop_name[260];
		{
			QWORD name_ptr = rx_read_i64(game_process, recv_prop + 0x28);
			rx_read_process(game_process, name_ptr, recv_prop_name, 260);
		}

		if (!strcmp(recv_prop_name, name))
		{
			return rx_read_i32(game_process, recv_prop + 0x4);
		}
	}

	return 0;
}

int main(void)
{
	int pid = GetApexProcessId();

	if (pid == 0)
	{
		printf("[-] r5apex.exe was not found\n");
		return 0;
	}

	rx_handle r5apex = rx_open_process(pid, RX_ALL_ACCESS);
	if (r5apex == 0)
	{
		printf("[-] unable to attach r5apex.exe\n");
		return 0;
	}

	printf("[+] r5apex.exe pid [%d]\n", pid);

	//
	// get base address
	// in case this function doesn't work, use QWORD base_module = 0x140000000;
	//
	QWORD base_module = GetApexBaseAddress(pid);
	if (base_module == 0)
	{
		return 0;
	}

	printf("[+] r5apex.exe base [0x%lx]\n", base_module);

	DWORD dwBulletSpeed = 0, dwBulletGravity = 0, dwMuzzle = 0, dwVisibleTime = 0;

	QWORD base_module_dump = rx_dump_module(r5apex, base_module);

	if (base_module_dump == 0)
	{
		printf("[-] failed to dump r5apex.exe\n");
		rx_close_handle(r5apex);
		return 0;
	}

	QWORD IClientEntityList = 0;
	{
		char pattern[] = "\x4C\x8B\x15\x00\x00\x00\x00\x33\xF6";
		char mask[] = "xxx????xx";

		// IClientEntityList = 0x1a203b8 + base_module + 0x280050;
		IClientEntityList = rx_scan_pattern(base_module_dump, pattern, mask, 10);
		if (IClientEntityList)
		{
			IClientEntityList = ResolveRelativeAddressEx(r5apex, IClientEntityList, 3, 7);
			IClientEntityList = IClientEntityList + 0x08;
		}
	}

	QWORD dwLocalPlayer = 0;
	{

		// 89 41 28 48 8B 05 ? ? ? ?
		char pattern[] = "\x89\x41\x28\x48\x8B\x05\x00\x00\x00\x00";
		char mask[] = "xxxxxx????";
		dwLocalPlayer = rx_scan_pattern(base_module_dump, pattern, mask, 11);
		if (dwLocalPlayer)
		{
			dwLocalPlayer = dwLocalPlayer + 0x03;
			dwLocalPlayer = ResolveRelativeAddressEx(r5apex, dwLocalPlayer, 3, 7);
		}
	}

	QWORD IInputSystem = 0;
	{
		// 48 8B 05 ? ? ? ? 48 8D 4C  24 20 BA 01 00 00 00 C7
		char pattern[] = "\x48\x8B\x05\x00\x00\x00\x00\x48\x8D\x4C\x24\x20\xBA\x01\x00\x00\x00\xC7";
		char mask[] = "xxx????xxxxxxxxxxx";

		IInputSystem = rx_scan_pattern(base_module_dump, pattern, mask, 19);
		IInputSystem = ResolveRelativeAddressEx(r5apex, IInputSystem, 3, 7);
		IInputSystem = IInputSystem - 0x10;
	}

	QWORD GetAllClasses = 0;
	{
		// 48 8B 05 ? ? ? ? C3 CC CC CC CC CC CC CC CC 48 89 74 24 20
		char pattern[] = "\x48\x8B\x05\x00\x00\x00\x00\xC3\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\x48\x89\x74\x24\x20";
		char mask[] = "xxx????xxxxxxxxxxxxxx";
		GetAllClasses = rx_scan_pattern(base_module_dump, pattern, mask, 22);
		GetAllClasses = ResolveRelativeAddressEx(r5apex, GetAllClasses, 3, 7);
		GetAllClasses = rx_read_i64(r5apex, GetAllClasses);
	}

	QWORD sensitivity = 0;
	{
		// sensitivity
		// 48 8B 05 ? ? ? ? F3 0F 10 3D ? ? ? ? F3 0F 10 70 68
		char pattern[] = "\x48\x8B\x05\x00\x00\x00\x00\xF3\x0F\x10\x3D\x00\x00\x00\x00\xF3\x0F\x10\x70\x68";
		char mask[] = "xxx????xxxx????xxxxx";
		sensitivity = rx_scan_pattern(base_module_dump, pattern, mask, 21);

		if (sensitivity)
		{
			sensitivity = ResolveRelativeAddressEx(r5apex, sensitivity, 3, 7);
			sensitivity = rx_read_i64(r5apex, sensitivity);
		}
	}

	{

		char pattern[] = "\x75\x0F\xF3\x44\x0F\x10\xBF\x00\x00\x00\x00";
		char mask[] = "xxxxxxx????";
		QWORD temp_address = rx_scan_pattern(base_module_dump, pattern, mask, 12);
		if (temp_address)
		{

			QWORD bullet_gravity = temp_address + 0x02;
			bullet_gravity = bullet_gravity + 0x05;

			QWORD bullet_speed = temp_address - 0x6D;
			bullet_speed = bullet_speed + 0x04;

			dwBulletSpeed = rx_read_i32(r5apex, bullet_speed);
			dwBulletGravity = rx_read_i32(r5apex, bullet_gravity);
		}
	}

	{
		char pattern[] = "\xF3\x0F\x10\x91\x00\x00\x00\x00\x48\x8D\x04\x40";
		char mask[] = "xxxx????xxxx";

		QWORD temp_address = rx_scan_pattern(base_module_dump, pattern, mask, 13);
		if (temp_address)
		{
			temp_address = temp_address + 0x04;
			dwMuzzle = rx_read_i32(r5apex, temp_address);
		}
	}

	{
		// 48 8B CE  ? ? ? ? ? 84 C0 0F 84 BA 00 00 00
		char pattern[] = "\x48\x8B\xCE\x00\x00\x00\x00\x00\x84\xC0\x0F\x84\xBA\x00\x00\x00";
		char mask[] = "xxx?????xxxxxxxx";
		QWORD temp_address = rx_scan_pattern(base_module_dump, pattern, mask, 17);
		if (temp_address)
		{
			temp_address = temp_address + 0x10;
			dwVisibleTime = rx_read_i32(r5apex, temp_address + 0x4);
		}
	}

	rx_free_module(base_module_dump);

	while (GetAllClasses)
	{

		QWORD recv_table = rx_read_i64(r5apex, GetAllClasses + 0x18);
		QWORD recv_name = rx_read_i64(r5apex, recv_table + 0x4C8);

		char name[260];
		rx_read_process(r5apex, recv_name, name, 260);

		if (!strcmp(name, "DT_Player"))
		{
			m_iHealth = dump_table(r5apex, recv_table, "m_iHealth");
			m_iViewAngles = dump_table(r5apex, recv_table, "m_ammoPoolCapacity") - 0x14;
			m_bZooming = dump_table(r5apex, recv_table, "m_bZooming");
			m_lifeState = dump_table(r5apex, recv_table, "m_lifeState");
			m_iCameraAngles = dump_table(r5apex, recv_table, "m_zoomFullStartTime") + 0x2EC;
		}

		if (!strcmp(name, "DT_BaseEntity"))
		{
			m_iTeamNum = dump_table(r5apex, recv_table, "m_iTeamNum");
			m_vecAbsOrigin = 0x014c;
		}

		if (!strcmp(name, "DT_BaseCombatCharacter"))
		{
			m_iWeapon = dump_table(r5apex, recv_table, "m_latestPrimaryWeapons");
		}

		if (!strcmp(name, "DT_BaseAnimating"))
		{
			m_iBoneMatrix = dump_table(r5apex, recv_table, "m_nForceBone") + 0x50 - 0x8;
		}

		if (!strcmp(name, "DT_WeaponX"))
		{
			m_playerData = dump_table(r5apex, recv_table, "m_playerData");
		}

		GetAllClasses = rx_read_i64(r5apex, GetAllClasses + 0x20);
	}

	DWORD previous_tick = 0;
	float lastvis_aim[70];
	memset(lastvis_aim, 0, sizeof(lastvis_aim));

	if (IClientEntityList == 0)
	{
		printf("[-] IClientEntityList not found\n");
		goto ON_EXIT;
	}

	if (dwLocalPlayer == 0)
	{
		printf("[-] dwLocalPlayer not found\n");
		goto ON_EXIT;
	}

	if (IInputSystem == 0)
	{
		printf("[-] IInputSystem not found\n");
		goto ON_EXIT;
	}

	if (sensitivity == 0)
	{
		printf("[-] sensitivity not found\n");
		goto ON_EXIT;
	}

	if (dwBulletSpeed == 0)
	{
		printf("[-] dwBulletSpeed not found\n");
		goto ON_EXIT;
	}

	if (dwBulletGravity == 0)
	{
		printf("[-] dwBulletGravity not found\n");
		goto ON_EXIT;
	}

	if (dwMuzzle == 0)
	{
		printf("[-] dwMuzzle not found\n");
		goto ON_EXIT;
	}

	if (dwVisibleTime == 0)
	{
		printf("[-] dwVisibleTime not found\n");
		goto ON_EXIT;
	}

	dwMuzzle = dwMuzzle - 0x04;

	printf("[+] IClientEntityList: %lx\n", IClientEntityList - base_module);
	printf("[+] dwLocalPlayer: %lx\n", dwLocalPlayer - base_module);
	printf("[+] IInputSystem: %lx\n", IInputSystem - base_module);
	printf("[+] sensitivity: %lx\n", sensitivity - base_module);
	printf("[+] dwBulletSpeed: %x\n", dwBulletSpeed);
	printf("[+] dwBulletGravity: %x\n", dwBulletGravity);
	printf("[+] dwMuzzle: %x\n", dwMuzzle);
	printf("[+] dwVisibleTime: %x\n", dwMuzzle);
	printf("[+] m_iHealth: %x\n", m_iHealth);
	printf("[+] m_iViewAngles: %x\n", m_iViewAngles);
	printf("[+] m_bZooming: %x\n", m_bZooming);
	printf("[+] m_iCameraAngles: %x\n", m_iCameraAngles);
	printf("[+] m_lifeState: %x\n", m_lifeState);
	printf("[+] m_iTeamNum: %x\n", m_iTeamNum);
	printf("[+] m_vecAbsOrigin: %x\n", m_vecAbsOrigin);
	printf("[+] m_iWeapon: %x\n", m_iWeapon);
	printf("[+] m_iBoneMatrix: %x\n", m_iBoneMatrix);
	printf("[+] m_playerData: %x\n", m_playerData);

	while (1)
	{
		if (!rx_process_exists(r5apex))
		{
			break;
		}

		QWORD localplayer = rx_read_i64(r5apex, dwLocalPlayer);

		if (localplayer == 0)
		{
			previous_tick = 0;
			memset(lastvis_aim, 0, sizeof(lastvis_aim));
			continue;
		}

		DWORD local_team = rx_read_i32(r5apex, localplayer + m_iTeamNum);

		float fl_sensitivity = rx_read_float(r5apex, sensitivity + 0x68);
		DWORD weapon_id = rx_read_i32(r5apex, localplayer + m_iWeapon) & 0xFFFF;
		QWORD weapon = GetClientEntity(r5apex, IClientEntityList, weapon_id - 1);

		float bulletSpeed = rx_read_float(r5apex, weapon + dwBulletSpeed);
		float bulletGravity = rx_read_float(r5apex, weapon + dwBulletGravity);

		vec3 muzzle;
		rx_read_process(r5apex, localplayer + dwMuzzle, &muzzle, sizeof(vec3));

		float target_fov = 360.0f;
		QWORD target_entity = 0;

		vec3 local_position;
		rx_read_process(r5apex, localplayer + m_vecAbsOrigin, &local_position, sizeof(vec3));

		for (int i = 0; i < 70; i++)
		{
			QWORD entity = GetClientEntity(r5apex, IClientEntityList, i);

			if (entity == 0)
				continue;

			if (entity == localplayer)
				continue;

			if (rx_read_i32(r5apex, entity + m_iHealth) == 0)
			{
				lastvis_aim[i] = 0;
				continue;
			}

			if (rx_read_i32(r5apex, entity + m_iTeamNum) == local_team)
			{
				continue;
			}

			if (rx_read_i32(r5apex, entity + m_lifeState) != 0)
			{
				lastvis_aim[i] = 0;
				continue;
			}

			vec3 head = GetBonePosition(r5apex, entity, 2);

			vec3 velocity;
			rx_read_process(r5apex, entity + m_vecAbsOrigin - 0xC, &velocity, sizeof(vec3));

			float fl_time = vec_distance(head, muzzle) / bulletSpeed;
			head.z += (700.0f * bulletGravity * 0.5f) * (fl_time * fl_time);

			velocity.x = velocity.x * fl_time;
			velocity.y = velocity.y * fl_time;
			velocity.z = velocity.z * fl_time;

			head.x += velocity.x;
			head.y += velocity.y;
			head.z += velocity.z;

			vec3 target_angle = CalcAngle(muzzle, head);
			vec3 breath_angles;

			rx_read_process(r5apex, localplayer + m_iViewAngles - 0x10, &breath_angles, sizeof(vec3));

			float last_visible = rx_read_float(r5apex, entity + dwVisibleTime);

			if (last_visible != 0.00f)
			{

				float fov = get_fov(breath_angles, target_angle);

				if (fov < target_fov && last_visible > lastvis_aim[i])
				{

					target_fov = fov;
					target_entity = entity;
					lastvis_aim[i] = last_visible;
				}
			}

#if GLOW_ESP == 1
			rx_write_i32(r5apex, entity + 0x262, 16256);
			rx_write_i32(r5apex, entity + 0x2dc, 1193322764);
			rx_write_i32(r5apex, entity + 0x3c8, 7);
			rx_write_i32(r5apex, entity + 0x3d0, 2);
#endif
		}

		if (target_entity && IsButtonDown(r5apex, IInputSystem, AIMKEY))
		{

			if (rx_read_i32(r5apex, target_entity + m_iHealth) == 0)
				continue;

			vec3 target_angle = {0, 0, 0};
			float fov = 360.0f;
			int bone_list[] = {2, 3, 5, 8};

			vec3 breath_angles;
			rx_read_process(r5apex, localplayer + m_iViewAngles - 0x10, &breath_angles, sizeof(vec3));

			for (int i = 0; i < 4; i++)
			{
				vec3 head = GetBonePosition(r5apex, target_entity, bone_list[i]);

				vec3 velocity;
				rx_read_process(r5apex, target_entity + m_vecAbsOrigin - 0xC, &velocity, sizeof(vec3));

				float fl_time = vec_distance(head, muzzle) / bulletSpeed;

				head.z += (700.0f * bulletGravity * 0.5f) * (fl_time * fl_time);

				velocity.x = velocity.x * fl_time;
				velocity.y = velocity.y * fl_time;
				velocity.z = velocity.z * fl_time;

				head.x += velocity.x;
				head.y += velocity.y;
				head.z += velocity.z;

				vec3 angle = CalcAngle(muzzle, head);
				float temp_fov = get_fov(breath_angles, angle);
				if (temp_fov < fov)
				{
					fov = temp_fov;
					target_angle = angle;
				}
			}

			DWORD weapon_id = rx_read_i32(r5apex, localplayer + m_iWeapon) & 0xFFFF;
			QWORD weapon = GetClientEntity(r5apex, IClientEntityList, weapon_id - 1);
			float zoom_fov = rx_read_float(r5apex, weapon + m_playerData + 0xb8);

			if (rx_read_i8(r5apex, localplayer + m_bZooming))
			{
				fl_sensitivity = (zoom_fov / 90.0f) * fl_sensitivity;
			}

			if (fov <= AIMFOV)
			{

				vec3 angles;
				angles.x = breath_angles.x - target_angle.x;
				angles.y = breath_angles.y - target_angle.y;
				angles.z = 0;
				vec_clamp(&angles);

				float x = angles.y;
				float y = angles.x;
				x = (x / fl_sensitivity) / 0.022f;
				y = (y / fl_sensitivity) / -0.022f;

				float sx = 0.0f, sy = 0.0f;

				float smooth = AIMSMOOTH;

				DWORD aim_ticks = 0;

				if (smooth >= 1.0f)
				{
					if (sx < x)
						sx = sx + 1.0f + (x / smooth);
					else if (sx > x)
						sx = sx - 1.0f + (x / smooth);
					else
						sx = x;

					if (sy < y)
						sy = sy + 1.0f + (y / smooth);
					else if (sy > y)
						sy = sy - 1.0f + (y / smooth);
					else
						sy = y;
					aim_ticks = (DWORD)(smooth / 100.0f);
				}
				else
				{
					sx = x;
					sy = y;
				}

				if (qabs((int)sx) > 100)
					continue;

				if (qabs((int)sy) > 100)
					continue;

				DWORD current_tick = rx_read_i32(r5apex, IInputSystem + 0xcd8);
				if (current_tick - previous_tick > aim_ticks)
				{
					previous_tick = current_tick;
					typedef struct
					{
						int x, y;
					} mouse_data;
					mouse_data data;

					data.x = (int)sx;
					data.y = (int)sy;
					rx_write_process(r5apex, IInputSystem + 0x1DB0, &data, sizeof(data));
				}
			}
		}
	}

ON_EXIT:
	rx_close_handle(r5apex);
}

QWORD rx_read_i64(rx_handle process, QWORD address)
{
	QWORD buffer = 0;
	rx_read_process(process, address, &buffer, sizeof(buffer));
	return buffer;
}

DWORD rx_read_i32(rx_handle process, QWORD address)
{
	DWORD buffer = 0;
	rx_read_process(process, address, &buffer, sizeof(buffer));
	return buffer;
}

WORD rx_read_i16(rx_handle process, QWORD address)
{
	WORD buffer = 0;
	rx_read_process(process, address, &buffer, sizeof(buffer));
	return buffer;
}

BYTE rx_read_i8(rx_handle process, QWORD address)
{
	BYTE buffer = 0;
	rx_read_process(process, address, &buffer, sizeof(buffer));
	return buffer;
}

float rx_read_float(rx_handle process, QWORD address)
{
	float buffer = 0;
	rx_read_process(process, address, &buffer, sizeof(buffer));
	return buffer;
}

BOOL rx_write_i32(rx_handle process, QWORD address, DWORD value)
{
	return rx_write_process(process, address, &value, sizeof(value)) == sizeof(value);
}

QWORD ResolveRelativeAddressEx(
    rx_handle process,
    QWORD Instruction,
    DWORD OffsetOffset,
    DWORD InstructionSize)
{

	QWORD Instr = (QWORD)Instruction;
	DWORD RipOffset = rx_read_i32(process, Instr + OffsetOffset);
	QWORD ResolvedAddr = (QWORD)(Instr + InstructionSize + RipOffset);
	return ResolvedAddr;
}

QWORD rx_dump_module(rx_handle process, QWORD base)
{
	QWORD a0, a1, a2, a3 = 0;
	char *a4;

	a0 = base;
	if (a0 == 0)
		return 0;

	a1 = rx_read_i32(process, a0 + 0x03C) + a0;
	if (a1 == a0)
	{
		return 0;
	}

	a2 = rx_read_i32(process, a1 + 0x050);
	if (a2 < 8)
		return 0;

	a4 = (char *)malloc(a2 + 24);

	*(QWORD *)(a4) = base;
	*(QWORD *)(a4 + 8) = a2;
	*(QWORD *)(a4 + 16) = a3;

	a4 += 24;

	QWORD image_dos_header = base;
	QWORD image_nt_header = rx_read_i32(process, image_dos_header + 0x03C) + image_dos_header;

	DWORD headers_size = rx_read_i32(process, image_nt_header + 0x54);
	rx_read_process(process, image_dos_header, a4, headers_size);

	unsigned short machine = rx_read_i16(process, image_nt_header + 0x4);

	QWORD section_header = machine == 0x8664 ? image_nt_header + 0x0108 : image_nt_header + 0x00F8;

	for (WORD i = 0; i < rx_read_i16(process, image_nt_header + 0x06); i++)
	{

		QWORD section = section_header + (i * 40);
		QWORD local_virtual_address = base + rx_read_i32(process, section + 0x0c);
		DWORD local_virtual_size = rx_read_i32(process, section + 0x8);
		QWORD target_virtual_address = (QWORD)a4 + rx_read_i32(process, section + 0xc);
		rx_read_process(process, local_virtual_address, (PVOID)target_virtual_address, local_virtual_size);
	}

	return (QWORD)a4;
}

void rx_free_module(QWORD dumped_module)
{
	dumped_module -= 24;
	free((void *)dumped_module);
}

BOOL bDataCompare(const BYTE *pData, const BYTE *bMask, const char *szMask)
{

	for (; *szMask; ++szMask, ++pData, ++bMask)
		if (*szMask == 'x' && *pData != *bMask)
			return 0;

	return (*szMask) == 0;
}

QWORD FindPatternEx(QWORD dwAddress, QWORD dwLen, BYTE *bMask, char *szMask)
{

	if (dwLen <= 0)
		return 0;
	for (QWORD i = 0; i < dwLen; i++)
		if (bDataCompare((BYTE *)(dwAddress + i), bMask, szMask))
			return (QWORD)(dwAddress + i);

	return 0;
}

QWORD rx_scan_pattern(QWORD dumped_module, PCSTR pattern, PCSTR mask, QWORD length)
{
	QWORD ret = 0;
	QWORD a0;

	if (dumped_module == 0)
		return 0;

	dumped_module -= 24;
	a0 = *(QWORD *)(dumped_module);
	dumped_module += 24;

	QWORD image_dos_header = dumped_module;
	QWORD image_nt_header = *(DWORD *)(image_dos_header + 0x03C) + image_dos_header;

	unsigned short machine = *(unsigned short *)(image_nt_header + 0x4);

	QWORD section_header = machine == 0x8664 ? image_nt_header + 0x0108 : image_nt_header + 0x00F8;

	for (WORD i = 0; i < *(unsigned short *)(image_nt_header + 0x06); i++)
	{

		QWORD section = section_header + (i * 40);
		QWORD section_address = image_dos_header + *(DWORD *)(section + 0x14);
		DWORD section_size = *(DWORD *)(section + 0x10);
		DWORD section_characteristics = *(DWORD *)(section + 0x24);

		if (section_characteristics & 0x00000020)
		{
			QWORD address = FindPatternEx(section_address, section_size - length, (BYTE *)pattern, (char *)mask);
			if (address)
			{
				ret = (address - dumped_module) + a0;
				break;
			}
		}
	}
	return ret;
}
