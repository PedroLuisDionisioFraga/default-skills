# {{COMPONENT_TITLE}}

[![Component Registry](https://components.espressif.com/components/{{NAMESPACE}}/{{COMPONENT_NAME}}/badge.svg)](https://components.espressif.com/components/{{NAMESPACE}}/{{COMPONENT_NAME}})

{{DESCRIPTION}}

## Features

- TODO: list the main features of the component.

## Installation

Add the component to your project from the [ESP Component Registry](https://components.espressif.com/components/{{NAMESPACE}}/{{COMPONENT_NAME}}):

```bash
idf.py add-dependency "{{NAMESPACE}}/{{COMPONENT_NAME}}^{{VERSION}}"
```

Or add it manually to your `main/idf_component.yml`:

```yaml
dependencies:
  {{NAMESPACE}}/{{COMPONENT_NAME}}: "^{{VERSION}}"
```

## Usage

```c
#include "{{COMPONENT_NAME}}.h"

void app_main(void)
{
  // TODO: minimal usage snippet.
}
```

## Examples

| Example | Description |
|---------|-------------|
| [basic](examples/basic) | Minimal usage of the component. |

Create a project from an example:

```bash
idf.py create-project-from-example "{{NAMESPACE}}/{{COMPONENT_NAME}}:basic"
```

## API Reference

See [include/{{COMPONENT_NAME}}.h](include/{{COMPONENT_NAME}}.h) for the full public API.

## License

[MIT](LICENSE)
