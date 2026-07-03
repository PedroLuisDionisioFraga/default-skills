# Component root CMakeLists.txt — this repository IS the component.
# No project() boilerplate here; consumers pull this via the ESP Component Registry.
idf_component_register(SRCS "src/{{COMPONENT_NAME}}.c"
                       INCLUDE_DIRS "include")
# Add REQUIRES/PRIV_REQUIRES for IDF components your code needs, e.g.:
#   REQUIRES driver esp_timer
# (REQUIRES = needed by public headers; PRIV_REQUIRES = needed only by .c files)
