################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Source/planner/implements/TrajectoryLocalPlanner/TrajectoryLocalPlanner.cpp 

OBJS += \
./Source/planner/implements/TrajectoryLocalPlanner/TrajectoryLocalPlanner.o 

CPP_DEPS += \
./Source/planner/implements/TrajectoryLocalPlanner/TrajectoryLocalPlanner.d 


# Each subdirectory must supply rules for building sources it contributes
Source/planner/implements/TrajectoryLocalPlanner/%.o: ../Source/planner/implements/TrajectoryLocalPlanner/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	arm-openwrt-linux-muslgnueabi-g++ -I$(SENAVICOMMON_PATH)/Source -I$(STAGING_DIR)/usr/include/allwinner/include/ -I$(STAGING_DIR)/usr/include/libsgbot/ -I$(STAGING_DIR)/usr/include/allwinner -O0 -g3 -Wall -D BOOST_LOG_DYN_LINK -D logLevel=0 -c -fmessage-length=0 -std=gnu++11 -DBOOST_LOG_DYN_LINK -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

