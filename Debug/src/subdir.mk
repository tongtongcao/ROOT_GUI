################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/ROOT_GUI.cpp \
../src/UserGuiGeneralDialogProcess.cpp \
../src/UserGuiMainFrame.cpp 

OBJS += \
./src/ROOT_GUI.o \
./src/UserGuiGeneralDialogProcess.o \
./src/UserGuiMainFrame.o 

CPP_DEPS += \
./src/ROOT_GUI.d \
./src/UserGuiGeneralDialogProcess.d \
./src/UserGuiMainFrame.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	g++ -std=c++0x -pthread -I"/home/newdriver/Source/ROOT_CERN/root-6.08.00/root-6.08.00-build/include" -I"/home/newdriver/Source/CODA/2.6.2/Linux-x86_64/include" -O2 -g3 -Wall -std=c++11 -pthread -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


