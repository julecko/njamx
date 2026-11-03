# Adds or removes a single directory from the User or Machine PATH
# registry value, called by installer.nsi during install/uninstall.
#
# Kept out of the .nsi itself because NSIS's own string-splitting idioms
# are easy to get subtly wrong (the NSIS wiki's own EnvVarUpdate.nsh, the
# usual copy-paste answer for this, documents a corruption bug when one
# PATH entry is a substring of another); PowerShell's -split/-join on an
# exact-match array avoids that entirely.
param(
    [Parameter(Mandatory = $true)]
    [string] $Dir,

    [Parameter(Mandatory = $true)]
    [ValidateSet('User', 'Machine')]
    [string] $Scope,

    [Parameter(Mandatory = $true)]
    [ValidateSet('Add', 'Remove')]
    [string] $Action
)

$current = [Environment]::GetEnvironmentVariable('Path', $Scope)
if ($null -eq $current) {
    $current = ''
}

$parts = @($current -split ';' | Where-Object { $_ -ne '' })

if ($Action -eq 'Add') {
    if ($parts -notcontains $Dir) {
        $parts += $Dir
    }
} else {
    $parts = @($parts | Where-Object { $_ -ne $Dir })
}

[Environment]::SetEnvironmentVariable('Path', ($parts -join ';'), $Scope)
