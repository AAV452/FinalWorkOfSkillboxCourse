cmake_minimum_required(VERSION 3.17)

#${CMAKE_PROJECT_NAME} - имя проекта, определённое в самом верхнем CMakeLists.txt
#${PROJECT_NAME} - имя текущего проекта, определённое в данном CMakeLists.txt или если нет, то в родительском

#${CMAKE_SOURCE_DIR} - путь к самому первому CMakeLists.txt
#${CMAKE_CURRENT_SOURCE_DIR} - путь к текущему CMakeLists.txt
#${CMAKE_SOURCE_DIR}/include - это то же, что и - ${CMAKE_CURRENT_SOURCE_DIR}/../include

set(MY_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include) #берём путь к папке с хедерами

#подключаем папку nlohmann_json с JSON
set(JSON_BuildTests OFF CACHE INTERNAL "")
set(JSON_DIR ${CMAKE_SOURCE_DIR}/nlohmann_json)
include_directories(JSON_DIR)


#подключаем все необходимые файлы для создания исполняемого .exe-файла:
add_executable( ${CMAKE_PROJECT_NAME} # имя .ехе-файла будет равно названию проекта
			    mainProject.cpp
			    converterJSON.cpp  ${MY_INCLUDE_DIR}/converterJSON.h
				invertedIndex.cpp  ${MY_INCLUDE_DIR}/invertedIndex.h
				searchServer.cpp   ${MY_INCLUDE_DIR}/searchServer.h )
																		

#указываем пути к заголовочным файлам, чтобы не указывать полные пути к ним в 
#коде при инклюде #include "nlohmann/json.hpp":
target_include_directories( ${CMAKE_PROJECT_NAME}
			         PUBLIC ${MY_INCLUDE_DIR}
			  		        ${JSON_DIR})

#указываем, с какими библиотеками нужно линковать исполняемый .exe-файл:
#PRIVATE - библиотека нужна только для сборки данной цели
#PUBLIC - библиотека нужна и самой цели и тем, кто её использует (думаю, для тестов надо???)
target_link_libraries( ${CMAKE_PROJECT_NAME} PUBLIC nlohmann_json::nlohmann_json)