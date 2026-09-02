include(CPM)

CPMAddPackage(
        NAME EACP
        GITHUB_REPOSITORY eyalamirmusic/eacp
        GIT_TAG main)

# Every source file in this repository is compiled into a JUCE target, under
# JUCE's recommended warning set — which is a good deal stricter than the one
# eacp holds itself to (-Wshadow, -Wswitch-enum, -Wsign-conversion). Left alone,
# every translation unit that includes an eacp header reports a dozen warnings
# from inside eacp, and the ones from our own code are lost among them.
#
# SYSTEM on the two targets that publish the include root (a CMake 3.25
# property; this project already requires 3.31) is the whole fix: eacp's headers
# stop being warned about, and nothing our own sources do is loosened.
foreach (eacp_target IN ITEMS eacp-core eacp-simd)
    if (TARGET ${eacp_target})
        set_target_properties(${eacp_target} PROPERTIES SYSTEM ON)
    endif ()
endforeach ()
