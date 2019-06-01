
#include "MiraCL.h"

#if USE_OPENCL

// ref. http://the-united-front.blogspot.jp/search/label/OpenCL%E3%81%AB%E3%82%88%E3%82%8BGPGPU

static std::string vtos(std::vector<char> const &vec)
{
	if (vec.empty()) return std::string();
	char const *left = &vec[0];
	char const *right = left + vec.size();
	while (left < right && right[-1] == 0) right--;
	return std::string(left, right);
}

void MiraCL::GetPlatformIDs(std::vector<cl_platform_id> *out)
{
	out->clear();

	cl_int result;

	cl_uint count;

	result = clGetPlatformIDs(0, nullptr, &count);
	if (result != CL_SUCCESS){
		fprintf(stderr, "When searching number of platforms\n");
		fprintf(stderr, "clGetPlatformIDs() was failed. return == %d\n", result);
		return;
	}

	out->resize(count);

	result = clGetPlatformIDs(count, &out->at(0), nullptr);
	if (result != CL_SUCCESS){
		fprintf(stderr, "When trying to get platform IDs\n");
		fprintf(stderr, "clGetPlatformIDs() failed. return == %d\n", result);
		return;
	}
}

std::string MiraCL::GetPlatformInfo(cl_platform_id platform_id, cl_platform_info param)
{
	cl_int result;
	size_t required_buff_size;

	result = clGetPlatformInfo(platform_id, param, 0, nullptr, &required_buff_size);
	if (result != CL_SUCCESS){
		fprintf(stderr, "When trying to get platform info size\n");
		fprintf(stderr, "clGetPlatformInfo() failed. return == %d\n", result);
		return nullptr;
	}
	if (required_buff_size < 1) return std::string();

	std::vector<char> buff(required_buff_size);

	result = clGetPlatformInfo(platform_id, param, buff.size(), &buff[0], nullptr);
	if (result != CL_SUCCESS){
		fprintf(stderr, "When trying to get platform info\n");
		fprintf(stderr, "clGetPlatformInfo() failed. return == %d\n", result);
		return nullptr;
	}

	char const *left = &buff[0];
	char const *right = left + buff.size();
	while (left < right && right[-1] == 0) right--;
	return std::string(left, right);
}

void MiraCL::GetDeviceIDs(cl_platform_id platform_id, cl_device_info param, std::vector<cl_device_id> *out)
{
	out->clear();

	cl_uint count;
	cl_int result;

	result = clGetDeviceIDs( platform_id, param, 0, nullptr, &count);
	if( result != CL_SUCCESS ){
		fprintf(stderr, "When trying to get number of devices\n");
		fprintf(stderr, "clGetDeviceIDs() failed. return == %d\n", result);
		return;
	}
	if (count < 1) return;

	out->resize(count);

	result = clGetDeviceIDs( platform_id, param, count, &out->at(0), 0 );
	if( result != CL_SUCCESS ){
		fprintf(stderr, "When trying to get device IDs\n");
		fprintf(stderr, "clGetDeviceIDs() failed. return == %d\n", result);
		return;
	}
}

bool MiraCL::GetDeviceInfo(cl_device_id device_id, cl_device_info param, std::vector<char> *out)
{
	out->clear();

	cl_int result;

	size_t required_buff_size;
	result = clGetDeviceInfo( device_id, param, 0, nullptr, &required_buff_size );
	if (result != CL_SUCCESS){
		fprintf(stderr, "When trying to get device info size\n");
		fprintf(stderr, "clGetDeviceInfo() failed. return == %d\n", result);
		return false;
	}
	if (required_buff_size < 1) return false;

	out->resize(required_buff_size);

	result = clGetDeviceInfo( device_id, param, required_buff_size, &out->at(0), nullptr );
	if (result != CL_SUCCESS){
		fprintf(stderr, "When trying to get platform info\n");
		fprintf(stderr, "clGetPlatformInfo() failed. return == %d\n", result);
		return false;
	}

	return true;
}

std::string MiraCL::GetProgramBuildLog(cl_program program, cl_device_id device_id)
{
	size_t len;
	clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, 0, nullptr, &len);
	std::vector<char> log(len);
	char *left = &log[0];
	clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, len, left, nullptr);
	char *right = left + len;
	while (left < right && right[-1] == 0) right--;
	return std::string(left, right);
}

std::string MiraCL::GetDeviceInfoString(cl_device_id device_id, cl_device_info param)
{
	std::string name;
	std::vector<char> vec;
	if (GetDeviceInfo(device_id, param, &vec)) {
		name = vtos(vec);
	}
	return name;
}

std::string MiraCL::GetDeviceName(cl_device_id device_id)
{
	return GetDeviceInfoString(device_id, CL_DEVICE_NAME);
}

std::string MiraCL::GetDeviceVendor(cl_device_id device_id)
{
	return GetDeviceInfoString(device_id, CL_DEVICE_VENDOR);
}

std::string MiraCL::GetDeviceDriverVersion(cl_device_id device_id)
{
	return GetDeviceInfoString(device_id, CL_DRIVER_VERSION);
}

template <typename T> inline T MiraCL::GetDeviceInfoT(cl_device_id device_id, cl_device_info param)
{
	std::vector<char> vec;
	if (GetDeviceInfo(device_id, param, &vec)) {
		if (vec.size() == sizeof(T)) {
			return *(T *)&vec[0];
		}
	}
	return 0;
}

bool MiraCL::GetDeviceInfoBool(cl_device_id device_id, cl_device_info param)
{
	return GetDeviceInfoT<cl_bool>(device_id, param) != CL_FALSE;
}

bool MiraCL::GetDeviceAvailable(cl_device_id device_id)
{
	return GetDeviceInfoBool(device_id, CL_DEVICE_AVAILABLE);
}

bool MiraCL::GetDeviceCompilerAvailable(cl_device_id device_id)
{
	return GetDeviceInfoBool(device_id, CL_DEVICE_COMPILER_AVAILABLE);
}

bool MiraCL::GetDeviceEndianLittle(cl_device_id device_id)
{
	return GetDeviceInfoBool(device_id, CL_DEVICE_ENDIAN_LITTLE);
}

MiraCL::Vendor MiraCL::GetDeviceVendorID(cl_device_id device_id)
{
	return (Vendor)GetDeviceInfoT<cl_uint>(device_id, CL_DEVICE_VENDOR_ID);
}

cl_platform_id MiraCL::GetDevicePlatformID(cl_device_id device_id)
{
	return GetDeviceInfoT<cl_platform_id>(device_id, CL_DEVICE_PLATFORM);
}

cl_ulong MiraCL::GetDeviceGlobalMemorySize(cl_device_id device_id)
{
	return GetDeviceInfoT<cl_ulong>(device_id, CL_DEVICE_GLOBAL_MEM_SIZE);
}

std::string MiraCL::GetPlatformProfile(cl_platform_id platform_id)
{
	return GetPlatformInfo(platform_id, CL_PLATFORM_PROFILE);
}

std::string MiraCL::GetPlatformVersion(cl_platform_id platform_id)
{
	return GetPlatformInfo(platform_id, CL_PLATFORM_VERSION);
}

std::string MiraCL::GetPlatformName(cl_platform_id platform_id)
{
	return GetPlatformInfo(platform_id, CL_PLATFORM_NAME);
}

std::string MiraCL::GetPlatformVendor(cl_platform_id platform_id)
{
	return GetPlatformInfo(platform_id, CL_PLATFORM_VENDOR);
}

//

bool MiraCL::open()
{
	{
		std::vector<cl_platform_id> platformids;
		GetPlatformIDs(&platformids);
		for (cl_platform_id id : platformids) {
			std::vector<cl_device_id> devs;
			GetDeviceIDs(id, CL_DEVICE_TYPE_GPU, &devs);
			if (devs.size() > 0) {
				platform_id = id;
				cl_device_id did = devs[0];
				Vendor vid = GetDeviceVendorID(did);
				if (vid == Vendor::NVIDIA) {
					device_id = did;
				}
			}
		}
	}
	if (!device_id) return false;

	cl_int ret;
	context = clCreateContext(nullptr, 1, &device_id, nullptr, nullptr, &ret);
	command_queue = clCreateCommandQueue(context, device_id, 0, &ret);
	return true;
}

void MiraCL::close()
{
	if (command_queue) clReleaseCommandQueue(command_queue);
	if (context) clReleaseContext(context);
	command_queue = 0;
	context = 0;
}

void MiraCL::flush()
{
	clFlush(command_queue);
}

void MiraCL::finish()
{
	clFinish(command_queue);
}

//

void MiraCL::Program::build(MiraCL *cl, const std::string &source, std::string const &method)
{
	release();
	this->cl = cl;
	cl_int ret;
	char const *p = source.c_str();
	size_t source_size = source.size();
	program = clCreateProgramWithSource(cl->context, 1, (const char **)&p, (size_t const *)&source_size, &ret);
	ret = clBuildProgram(program, 1, &cl->device_id, nullptr, nullptr, nullptr);
	if (ret == CL_BUILD_PROGRAM_FAILURE) {
		std::string s = GetProgramBuildLog(program, cl->device_id);
		qDebug(s.c_str());
		return;
	}
	kernel = clCreateKernel(program, method.c_str(), &ret);
}

MiraCL::Program::Program()
{
}

MiraCL::Program::~Program()
{
	release();
}

void MiraCL::Program::release()
{
	if (kernel) clReleaseKernel(kernel);
	if (program) clReleaseProgram(program);
	kernel = 0;
	program = 0;
}

cl_int MiraCL::Program::setarg_(cl_uint index, size_t len, void const *ptr)
{
	return clSetKernelArg(kernel, index, len, ptr);

}

template <typename T> cl_int MiraCL::Program::setarg_(cl_uint index, T const *value)
{
	return setarg_(index, sizeof(*value), value);
}

cl_int MiraCL::Program::arg(cl_uint index, MiraCL::Buffer *buf)
{
	return setarg_(index, sizeof(cl_mem), &buf->clmem);
}

cl_int MiraCL::Program::arg(cl_uint index, cl_int *value)
{
	return setarg_(index, value);
}

cl_int MiraCL::Program::arg(cl_uint index, cl_uint *value)
{
	return setarg_(index, value);
}

cl_int MiraCL::Program::arg(cl_uint index, cl_float *value)
{
	return setarg_(index, value);
}

cl_int MiraCL::Program::arg(cl_uint index, cl_double *value)
{
	return setarg_(index, value);
}

cl_int MiraCL::Program::run(size_t g0)
{
	size_t global_item_size[] = {g0};
	return clEnqueueNDRangeKernel(cl->command_queue, kernel, 1, nullptr, global_item_size, nullptr, 0, nullptr, nullptr);
}

cl_int MiraCL::Program::run(size_t g0, size_t g1)
{
	size_t global_item_size[] = {g0, g1};
	return clEnqueueNDRangeKernel(cl->command_queue, kernel, 2, nullptr, global_item_size, nullptr, 0, nullptr, nullptr);
}

cl_int MiraCL::Program::run(size_t g0, size_t g1, size_t g2)
{
	size_t global_item_size[] = {g0, g1, g2};
	return clEnqueueNDRangeKernel(cl->command_queue, kernel, 3, nullptr, global_item_size, nullptr, 0, nullptr, nullptr);
}

//

MiraCL::Buffer::Buffer(MiraCL *cl)
	: cl(cl)
{
}

MiraCL::Buffer::~Buffer()
{
	clear();
}

void MiraCL::Buffer::alloc(size_t n)
{
	clear();
	cl_int ret;
	clmem = clCreateBuffer(cl->context, CL_MEM_READ_WRITE, n, nullptr, &ret);
}

void MiraCL::Buffer::clear()
{
	if (clmem) {
		clReleaseMemObject(clmem);
		clmem = 0;
	}
}

void MiraCL::Buffer::write(size_t offset, size_t length, const void *ptr)
{
	clEnqueueWriteBuffer(cl->command_queue, clmem, CL_TRUE, offset, length, ptr, 0, nullptr, nullptr);
}

void MiraCL::Buffer::read(size_t offset, size_t length, void *ptr)
{
	clEnqueueReadBuffer(cl->command_queue, clmem, CL_TRUE, offset, length, ptr, 0, nullptr, nullptr);
}


#endif


