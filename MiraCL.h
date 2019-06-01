#ifndef MIRACL_H
#define MIRACL_H

#define USE_OPENCL 0

#if USE_OPENCL
#include <cl/cl.h>
#include <vector>
#include <string>
#include <QFile>


class MiraCL {
private:
	cl_platform_id platform_id = 0;
	cl_device_id device_id = 0;
	cl_context context = 0;
	cl_command_queue command_queue = 0;
	static std::string GetDeviceInfoString(cl_device_id device_id, cl_device_info param);
	template <typename T> static inline T GetDeviceInfoT(cl_device_id device_id, cl_device_info param);
	static bool GetDeviceInfoBool(cl_device_id device_id, cl_device_info param);
	static std::string GetPlatformInfo(cl_platform_id platform_id, std::vector<char> *out);
public:
	bool open();
	void close();

	cl_platform_id platformID() const
	{
		return platform_id;
	}

	cl_device_id deviceID() const
	{
		return device_id;
	}

	class Buffer;

	class Program {
	private:
		MiraCL *cl = 0;
		cl_program program = 0;
		cl_kernel kernel = 0;
		inline cl_int setarg_(cl_uint index, size_t len, void const *ptr);
		template <typename T> inline cl_int setarg_(cl_uint index, T const *value);
	public:
		Program();
		~Program();
		void build(MiraCL *cl, const std::string &source, const std::string &method);
		void release();
		cl_int arg(cl_uint index, Buffer *buf);
		cl_int arg(cl_uint index, cl_int *value);
		cl_int arg(cl_uint index, cl_uint *value);
		cl_int arg(cl_uint index, cl_float *value);
		cl_int arg(cl_uint index, cl_double *value);
		cl_int run(size_t g0);
		cl_int run(size_t g0, size_t g1);
		cl_int run(size_t g0, size_t g1, size_t g2);
	};

	class Buffer {
		friend class Program;
	private:
		MiraCL *cl;
		cl_mem clmem = 0;
	public:
		Buffer(MiraCL *cl = 0);
		~Buffer();
		void write(size_t offset, size_t length, void const *ptr);
		void read(size_t offset, size_t length, void *ptr);
		void alloc(size_t n);
		void clear();
	};

	void flush();

	void finish();

	enum class Vendor {
		AMD = 0x1002,
		NVIDIA = 0x10de,
		INTEL = 0x8086,
	};

	static void GetPlatformIDs(std::vector<cl_platform_id> *out);
	static std::string GetPlatformInfo(cl_platform_id platform_ID, cl_platform_info param);
	static void GetDeviceIDs(cl_platform_id platform_ID, cl_device_info param, std::vector<cl_device_id> *out);
	static bool GetDeviceInfo(cl_device_id device_ID, cl_device_info param, std::vector<char> *out);
	static std::string GetProgramBuildLog(cl_program program, cl_device_id device_id);
	static std::string GetDeviceName(cl_device_id device_id);
	static std::string GetDeviceVendor(cl_device_id device_id);
	static std::string GetDeviceDriverVersion(cl_device_id device_id);
	static bool GetDeviceEndianLittle(cl_device_id device_id);
	static bool GetDeviceAvailable(cl_device_id device_id);
	static bool GetDeviceCompilerAvailable(cl_device_id device_id);
	static Vendor GetDeviceVendorID(cl_device_id device_id);
	static cl_platform_id GetDevicePlatformID(cl_device_id device_id);
	static cl_ulong GetDeviceGlobalMemorySize(cl_device_id device_id);
	static std::string GetPlatformProfile(cl_platform_id platform_id);
	static std::string GetPlatformVersion(cl_platform_id platform_id);
	static std::string GetPlatformName(cl_platform_id platform_id);
	static std::string GetPlatformVendor(cl_platform_id platform_id);
};

#endif

#endif // MIRACL_H
