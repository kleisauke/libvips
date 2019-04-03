const downloadPrefix = 'https://github.com/libvips/build-win64-mxe/releases/download/v';

function update_links(version, variant, architecture, linking) {
  const suffix = linking === 'shared' ? '' : `-${linking}`;
  const binary = `vips-dev-${architecture}-${variant}-${version}${suffix}.zip`;

  const downloadEl = document.getElementById('download');
  downloadEl.href = `${downloadPrefix}${version}/${binary}`;
  downloadEl.title = binary;
}

function update_states(version, variant, architecture, linking) {
  [version, variant, architecture, linking].forEach((element) => {
    const input = document.querySelector(`input[value='${element}']`);
    const button = input.parentElement;
    for (const sibling of button.parentElement.children) {
      sibling.classList.remove('active');
    }
    button.classList.add('active');
  });
}

function detect_machine() {
  const parser = new UAParser();
  const result = parser.getResult();
  const cpuArchitecture = result.cpu.architecture;
  //const os = result.os.name;
  //if (os === 'Windows')

  let architecture;
  switch (cpuArchitecture) {
    case 'ia32':
      architecture = 'w32';
      break;
    case 'arm64':
      architecture = 'arm64';
      break;
    case 'amd64':
    default:
      architecture = 'w64';
      break;
  }

  const variant = 'web';
  const linking = architecture === 'arm64' ? 'static' : 'shared';

  const input = document.querySelector(`input[value='${architecture}']`);
  input.checked = true;

  const version = document.querySelectorAll("input[name='version']")[0].value;

  update_states(version, variant, architecture, linking);
  update_links(version, variant, architecture, linking);
}

detect_machine();

function checked_val(name) {
  return document.querySelector(`input[name='${name}']:checked`).value;
}

document.querySelectorAll('input').forEach(el => {
  el.addEventListener('change', () => {
    const version = checked_val('version');
    const architecture = checked_val('architecture');
    let variant = checked_val('variant');
    let linking = checked_val('linking');

    const staticLinkInputs = document.querySelectorAll("input[value^='static']");
    if (variant === 'all') {
      // Distributing statically linked binaries against GPL libraries, violates the GPL license.
      if (linking.startsWith('static')) {
        staticLinkInputs.forEach(e => e.checked = false);
        linking = 'shared';
        document.querySelector(`input[value='${linking}']`).checked = true;
      }

      staticLinkInputs.forEach(e => e.parentElement.classList.add('disabled'));

      // -all variant is not available for Windows Arm64.
      document.querySelector("input[value='arm64']").parentElement.classList.add('disabled');
    } else {
      staticLinkInputs.forEach(e => e.parentElement.classList.remove('disabled'));
      document.querySelector("input[value='arm64']").parentElement.classList.remove('disabled');

      // Only the static(-ffi) variants are available for Windows Arm64.
      if (architecture === 'arm64') {
        if (linking === 'shared') {
          document.querySelector(`input[value='${linking}']`).checked = false;
          linking = 'static';
          document.querySelector(`input[value='${linking}']`).checked = true;
        }

        document.querySelector("input[value='all']").parentElement.classList.add('disabled');
        document.querySelector("input[value='shared']").parentElement.classList.add('disabled');
      } else {
        document.querySelector("input[value='all']").parentElement.classList.remove('disabled');
        document.querySelector("input[value='shared']").parentElement.classList.remove('disabled');
      }
    }

    update_states(version, variant, architecture, linking);
    update_links(version, variant, architecture, linking);
  })
});
