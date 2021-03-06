# SDP Service Template for cookiecutter

A [cookiecutter](https://cookiecutter.readthedocs.io/en/latest/)
for rolling out new C++ Ice data services for ASKAP.

This template doesn't really do much, but it took almost no time to setup and
will save time by avoiding the tedious search and replace method of creating
a new application from an existing one.

## Updating
New template values can be added through definition in ./cookiecutter.json and
then adjusting the reference files. Refer to the cookiecutter documentation for
details.

## Usage
Install cookiecutter if you haven't already.
It works fine to install directly into the ASKAP Python environment used by
rbuild (created during bootstrapping). With the askap environment active

    $ pip install cookiecutter

Then, to create a new project from the template

    $ cookiecutter /home/daniel/code/askapsoft/Tools/cookiecutter_templates/sdp-service-template/

The new project will be created in a subdirectory of your current working
directory, so make sure to be in the desired location (or move the files after
generation).

### Template Fields
When you run cookiecutter, you will be presented with a series of prompts. At
each one you can enter the text for customising the generated project. With the
exception of the `year`, none of the default values should be used.

However, it might be interesting to generate a project using the default values
for everything. Inspecting the generated files should give you some insight into
how the template variables affect the output.

**user_name**: Your name. Added to file header comments as the file author.

**user_email**: Your full email address. Added to file header comments.

**year**: The current year.

**project_directory_name**: The top-level directory name for the project source.

**service_long_name**: A full text name of the project. E.G.: RFI Service. Used in
comments.

**namespace**: The final C++ namespace for your project. Appended to "askap::cp::"
which is the namespace used for other CP/SDP code.

**package_name**: The rbuild package name.

**service_class_name**: The C++ class name for the primary data service class. This
will produce 2 classes:

* **service_class_name**: Utility class that instantiates the Ice ServiceManager
  base class, wrapped around the service implementation.
* **service_class_nameImpl**: Service implementation. Inherits from the abstract C++
  class generated by Ice for your interface. This is the class that must
  implement the Ice service methods in your interface.

**cli_app_name**: The executable filename for the executable binary file.

**cli_app_class_name**: The class name of the wrapper class that contains the
entry-point code.

**ice_service_name**: The name of your Ice service (minus the I prefix). So if your
Ice interface is called IRfiService, you would set RfiService here.

**ice_interface_filename**: Name of your Ice interface file (minus the extension).

**ice_interface_namespace**: The bottom-level Ice module name that contains your
service interface. This value is appended to "askap::interfaces::".
Typically named after your service. E.G.: The Sky Model Service interface sits
in "askap::interfaces::skymodelservice".
