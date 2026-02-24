cmake_minimum_required(VERSION 3.17)

project(SkillboxSearchEngine_GTest) # название и версия текущего проекта тестов

##################### подключение Google Test из GitHub:
#библиотека Google Test используется напрямую из GitHub, поэтому необходимо добавить строки:
include(FetchContent)
FetchContent_Declare( googletest URL
					  #для CMake4.0.0 нужно использовать Google Test версии v1.14.x:
					  #на сайте https://github.com/google/googletest слева кнопка выбора версий:
					  https://github.com/google/googletest/archive/refs/heads/v1.14.x.zip)

include_directories(${gtest_SOURCE_DIR}/include${gtest_SOURCE_DIR})

#для корректной работы библиотеки c компилятором MinGW, необходимо добавить:
set(gtest_disable_pthreadson)

#выставим динамическое подключение библиотеки к проекту:
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

#делаем все настройки доступными для проекта:
FetchContent_MakeAvailable(googletest)

#указываем, что в проекте будут применятся тесты:
#enable_testing()

#см. в конце файла добавление библиотеки gtest_main к проекту:
##################### библиотека Google Test подключена



#подключим пути к заголовочным файлам, необходимым для создания исполняемого файла тестов:
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../include )

#для сборки исполняемого файла тестов подключим файлы, которые будут использоваться в нём:
add_executable( ${PROJECT_NAME} 
				test_1.cpp
                # ${CMAKE_CURRENT_SOURCE_DIR} - путь к текущему CMakeLists.txt
                # ../ - шаг назад по директориям
				#${CMAKE_CURRENT_SOURCE_DIR}/../src/mainProject.cpp
				#${CMAKE_CURRENT_SOURCE_DIR}/../src/converterJSON.cpp

                ${CMAKE_CURRENT_SOURCE_DIR}/../include/converterJSON.h
				${CMAKE_CURRENT_SOURCE_DIR}/../include/invertedIndex.h
				${CMAKE_CURRENT_SOURCE_DIR}/../src/invertedIndex.cpp
				${CMAKE_CURRENT_SOURCE_DIR}/../include/searchServer.h
				${CMAKE_CURRENT_SOURCE_DIR}/../src/searchServer.cpp
              )
#добавляем путь к папке в список путей, которые препроцессор будет использовать при поиске
#заголовоxных файлов, включаемых в коде не в форме относительного пути:            
target_include_directories( ${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include )



#### теперь добавим библиотеку gtest_main:
target_link_libraries( ${PROJECT_NAME} PRIVATE gtest_main )
include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME})
####
