#include "../commons.h"
#include "cmp.h"
#include "pgd.h"

#define MODULE_VERSION "fate-v3"

class FatePatcher {
  public:
    FatePatcher(PatchFsFile& patchFs, const fs::path& input, const fs::path& output, const std::vector<std::string>& args)
        : patchFs(patchFs), input(input), output(output), args(args), progress({1}) {
    }

    void patchExtra() {
        checkHash(5589170732003281493);
        // iso copy, cpk patching, final changes
        progress = Progress({0.09, 0.9, 0.01});
        commonInit();
        patchEboot();
        patchCpkFromManifest("PSP_GAME/USRDIR/data.cpk", [&](ByteBuffer& buf, i64 cpkOffset, i64 offset) {
            inputIso->seek(cpkOffset + offset);
            inputIso->readFully(buf);
        });
        spdlog::info("Finalize ISO patching");
        if (std::find(args.begin(), args.end(), "--useMaestro") != args.end()) {
            applyNameReplacements("PSP_GAME/USRDIR/data.cpk", "Maestro");
        }
        if (std::find(args.begin(), args.end(), "--useLowResRuby") != args.end()) {
            applyNameReplacements("PSP_GAME/SYSDIR/EBOOT.BIN", "LowResRuby-EBOOT");
        }
        commonFinish();
    }

    void patchCCC() {
        checkHash(2141807601829812257);
        // iso copy, dns decrypt, cpk patching, final changes
        progress = Progress({0.09, 0.1, 0.8, 0.01});
        commonInit();
        patchEboot();
        patchPmf("DEMO01.PMF");
        patchPmf("DEMO02.PMF");
        const std::string dnsPath = "PSP_GAME/INSDIR/GAME.DNS";
        std::unique_ptr<PgdStreamFile> pgd = decryptGameDns(dnsPath);
        patchCpkFromManifest(dnsPath, [&](ByteBuffer& buf, i64, i64 offset) mutable { pgd->decryptPartially(offset, buf); });
        spdlog::info("Finalize ISO patching");
        if (std::find(args.begin(), args.end(), "--useMeltryllis") != args.end()) {
            applyNameReplacements("PSP_GAME/INSDIR/GAME.DNS", "Meltryllis");
            applyNameReplacements("PSP_GAME/SYSDIR/EBOOT.BIN", "Meltryllis-EBOOT");
        }
        if (std::find(args.begin(), args.end(), "--useMaestro") != args.end()) {
            applyNameReplacements("PSP_GAME/INSDIR/GAME.DNS", "Maestro");
        }
        if (std::find(args.begin(), args.end(), "--useLowResRuby") != args.end()) {
            applyNameReplacements("PSP_GAME/SYSDIR/EBOOT.BIN", "LowResRuby-EBOOT");
        }
        relocateIsoFile(*inputIso, *outputIso, isoRecords, "PSP_GAME/INSDIR/ICON0.PNG");
        relocateIsoFile(*inputIso, *outputIso, isoRecords, "PSP_GAME/INSDIR/PIC1.PNG");
        commonFinish();
    }

  private:
    PatchFsFile& patchFs;
    const fs::path& input;
    const fs::path& output;
    const std::vector<std::string>& args;

    Progress progress;
    ByteBuffer isoPrimaryDescriptor;
    std::vector<IsoDirectoryRecord> isoRecords;
    std::unique_ptr<Stream> inputIso;
    std::unique_ptr<Stream> outputIso;

    void checkHash(u64 expectedHash) {
        spdlog::trace("Check input ISO hash");
        Stream iso(input, true);
        i32 hashSize = 4096 * ISO_SECTOR_SIZE;
        if (iso.length() <= hashSize) {
            bail("Input ISO is too small and is invalid");
        }
        ByteBuffer buf(hashSize);
        iso.readFully(buf);
        u64 hash = XXH64(buf.data(), buf.size(), 0);
        if (expectedHash != hash) {
            spdlog::error("ISO hash mismatch! Expected {} but got {}", expectedHash, hash);
            spdlog::error("You're trying to patch wrong ISO!");
            bail("Invalid input ISO hash");
        } else {
            spdlog::trace("ISO hash is {}", hash);
        }
    }

    void commonInit() {
        if (fs::exists(output)) {
            spdlog::warn("Output already exists and will be overwritten: {}", output.u8string());
        }

        checkModuleVersion();
        logTimestamp();

        spdlog::info("Read input ISO records");
        isoRecords = getIsoRecords(input);

        spdlog::info("Copy ISO to output location");
        fs::remove(output);
        copyFile(input, output, [& progress = progress](auto current, auto total) { progress.updatePart(current, total); });

        spdlog::trace("Open input ISO");
        inputIso = std::make_unique<Stream>(input, true);
        if (!inputIso->good()) {
            bail("Failed to open input ISO");
        }
        spdlog::trace("Open output ISO");
        outputIso = std::make_unique<Stream>(output);
        if (!outputIso->good()) {
            bail("Failed to open output ISO");
        }
        isoPrimaryDescriptor = stashIsoPrimaryDescriptor(*outputIso);
    }

    void commonFinish() {
        spdlog::trace("Update primary ISO descriptor");
        restoreIsoPrimaryDescriptor(*outputIso, isoPrimaryDescriptor);
        auto newLbaCount = outputIso->length() / ISO_SECTOR_SIZE;
        outputIso->seek(16 * ISO_SECTOR_SIZE + 80);
        outputIso->writeInt(newLbaCount);
        outputIso->writeIntB(newLbaCount);
        progress.updatePart(1, 1);
    }

    void logTimestamp() {
        ByteBuffer timestamp = patchFs.getFileContents("patch://timestamp.meta");
        std::string timestampStr = std::string(timestamp.begin(), timestamp.end());
        spdlog::info("Patch timestamp: {}", timestampStr);
    }

    void checkModuleVersion() {
        ByteBuffer version = patchFs.getFileContents("patch://module-version.meta");
        std::string versionStr = std::string(version.begin(), version.end());
        spdlog::info("Module version: {}, patch requires version: {}", MODULE_VERSION, versionStr);
        if (versionStr != MODULE_VERSION) {
            bail("Module version mismatch. This PatchFS can't be applied using this version of the patcher.");
        }
    }

    void patchEboot() {
        spdlog::info("Patching EBOOT.BIN");
        ByteBuffer ebootPatch = patchFs.getFileContents("patch://EBOOT.BIN");
        patchIsoFile(*outputIso, isoRecords, "PSP_GAME/SYSDIR/EBOOT.BIN", ebootPatch);
    }

    void patchPmf(const std::string& name) {
        spdlog::info("Patching {}", name);
        std::stringstream patchSs;
        patchSs << "patch://" << name;
        std::stringstream isoPathSs;
        isoPathSs << "PSP_GAME/USRDIR/MOVIE/" << name;
        ByteBuffer patch = patchFs.getFileContents(patchSs.str());
        patchIsoFile(*outputIso, isoRecords, isoPathSs.str(), patch);
    }

    std::unique_ptr<PgdStreamFile> decryptGameDns(const std::string& dnsPath) {
        spdlog::info("Decrypting GAME.DNS");
        seekToIsoFile(*outputIso, isoRecords, "PSP_GAME/SYSDIR/EBOOT.BIN");
        outputIso->skip(1812048);
        ByteBuffer key(16);
        outputIso->readFully(key);
        seekToIsoFile(*inputIso, isoRecords, dnsPath);
        seekToIsoFile(*outputIso, isoRecords, dnsPath);
        auto pgd = std::make_unique<PgdStreamFile>(*inputIso, inputIso->pos(), key, 2);
        pgd->decryptFully(progress, *outputIso);
        return pgd;
    }

    void patchCpkFromManifest(std::string cpkName, std::function<void(ByteBuffer&, i64, i64)> dataProvider) {
        spdlog::info("Patching CPK");
        ByteBuffer cpkManifestData = patchFs.getFileContents("patch://cpk-manifest.json");
        picojson::value cpkManifest;
        std::string manifestErr;
        picojson::parse(cpkManifest, cpkManifestData.begin(), cpkManifestData.end(), &manifestErr);
        if (!manifestErr.empty()) {
            spdlog::error("JSON parsing error: {}", manifestErr);
            bail("Failed to parse CPK patch manifest");
        }
        auto cpkRecord = seekToIsoFile(*outputIso, isoRecords, cpkName);
        auto cpkPosition = cpkRecord.lba * ISO_SECTOR_SIZE;
        auto cpkEntries = cpkManifest.get<picojson::array>();
        i32 currentEntry = 0;
        for (picojson::value value : cpkEntries) {
            picojson::object entry = value.get<picojson::object>();
            std::string relPath = entry["relPath"].get<std::string>();
            std::stringstream fsPath;
            fsPath << "cpk://";
            fsPath << relPath;
            ByteBuffer patch = patchFs.getFileContents(fsPath.str());
            spdlog::info("Patching CPK file {}", relPath);
            i64 oldOffset = entry["oldOffset"].get<i64>();
            i64 oldSize = entry["oldSize"].get<i64>();
            i64 oldExtractSize = entry["oldExtractSize"].get<i64>();
            spdlog::trace("Offset: {} (ISO absolute: {}), size: {}, extract size: {}", oldOffset, cpkPosition + oldOffset,
                          oldSize, oldExtractSize);
            ByteBuffer bytes(oldSize);
            dataProvider(bytes, cpkPosition, oldOffset);

            if (oldSize != oldExtractSize) {
                spdlog::trace("Decompress LAYLA file {}", relPath);
                bytes = decompressLayla(bytes);
            }
            if (endsWith(relPath, ".cmp") || endsWith(relPath, ".CMP")) {
                spdlog::trace("Decompress CMP file {}", relPath);
                bytes = decompressCmp(bytes);
            }
            spdlog::trace("Apply patch on {}", relPath);
            bytes = applyPatch(bytes, patch);

            spdlog::trace("Patch CPK metadata");
            i32 newSize = entry["size"].get<i64>();
            picojson::array writeNewSizeAtArr = entry["writeNewSizeAt"].get<picojson::array>();
            for (picojson::value value : writeNewSizeAtArr) {
                i64 writeAt = value.get<i64>();
                outputIso->seek(cpkPosition + writeAt);
                outputIso->writeIntB(newSize);
            }

            i64 newOffset = entry["offset"].get<i64>();
            picojson::array writeNewOffsetAt = entry["writeNewOffsetAt"].get<picojson::array>();
            for (picojson::value value : writeNewOffsetAt) {
                i64 writeAt = value.get<i64>();
                outputIso->seek(cpkPosition + writeAt);
                i64 newOffsetToc = newOffset - 2048;
                outputIso->writeLongB(newOffsetToc);
            }

            if (entry["newFileName"].is<std::string>()) {
                std::string newName = entry["newFileName"].get<std::string>();
                picojson::array writeNewFileNameAt = entry["writeNewFileNameAt"].get<picojson::array>();
                for (picojson::value value : writeNewFileNameAt) {
                    i64 writeAt = value.get<i64>();
                    outputIso->seek(cpkPosition + writeAt);
                    outputIso->writeString(newName);
                }
            }

            if ((i32)bytes.size() != newSize) {
                bail("Size mismatch with manifest after patching file");
            }

            spdlog::trace("Patch CPK data");
            outputIso->seek(cpkPosition + newOffset);
            outputIso->writeFully(bytes);
            currentEntry++;
            progress.updatePart(currentEntry, cpkEntries.size());
        }

        spdlog::trace("Align final CPK sector");
        outputIso->align(ISO_SECTOR_SIZE);
        auto newCpkSize = outputIso->pos() - cpkPosition;

        spdlog::trace("Update CPK ISO record");
        outputIso->seek(cpkRecord.isoOffset + 2 + 8);
        outputIso->writeInt(newCpkSize);
        outputIso->writeIntB(newCpkSize);
    }

    void applyNameReplacements(std::string fileName, std::string groupName) {
        spdlog::info("Applying name replacements for " + groupName);
        ByteBuffer namesManifestData = patchFs.getFileContents("patch://name-replacements.json");
        picojson::value namesManifest;
        std::string manifestErr;
        picojson::parse(namesManifest, namesManifestData.begin(), namesManifestData.end(), &manifestErr);
        if (!manifestErr.empty()) {
            spdlog::error("JSON parsing error: {}", manifestErr);
            bail("Failed to parse name replacement manifest");
        }
        auto fileRecord = seekToIsoFile(*outputIso, isoRecords, fileName);
        auto filePosition = fileRecord.lba * ISO_SECTOR_SIZE;

        auto replacement = namesManifest.get<picojson::object>()[groupName].get<picojson::object>();
        spdlog::trace("Found name replacement data");
        auto patterns = replacement["patterns"].get<picojson::array>();
        spdlog::trace("Processing patterns");
        for (picojson::value patternValue : patterns) {
            picojson::object pattern = patternValue.get<picojson::object>();
            std::string replaceWith = pattern["replaceWith"].get<std::string>();
            i32 clearSize = pattern["clearSize"].get<i64>();
            spdlog::trace("Processing pattern, replace with: {}", replaceWith);
            picojson::array offsets = pattern["offsets"].get<picojson::array>();
            for (picojson::value offsetValue : offsets) {
                outputIso->seek(filePosition + offsetValue.get<i64>());
                outputIso->writeFully(ByteBuffer(clearSize));
                outputIso->seek(filePosition + offsetValue.get<i64>());
                outputIso->writeString(replaceWith, replaceWith.size());
            }
        }
        spdlog::trace("Processing patterns completed");

        auto pakFiles = replacement["pakFiles"].get<picojson::array>();
        spdlog::trace("Processing files");
        for (picojson::value pakFileValue : pakFiles) {
            picojson::object pakFile = pakFileValue.get<picojson::object>();
            std::string patchPath = pakFile["patchPath"].get<std::string>();
            i64 offset = pakFile["offset"].get<i64>();
            i32 size = pakFile["size"].get<i64>();
            spdlog::trace("Processing patch path: {}, file offset {}, size {}", patchPath, filePosition + offset, size);
            ByteBuffer buf(size);
            ByteBuffer patch = patchFs.getFileContents(patchPath);
            outputIso->seek(filePosition + offset);
            outputIso->readFully(buf);
            ByteBuffer bufPatched = applyPatch(buf, patch);
            if (bufPatched.size() != buf.size()) {
                bail("Name replacement patched file size mismatch. Name replacement is invalid.");
            }
            outputIso->seek(filePosition + offset);
            outputIso->writeFully(bufPatched);
        }
        spdlog::trace("Processing files completed");
    }
};

void moduleBootup(PatchFsFile& patchFs, const fs::path& input, const fs::path& output,
                  const std::vector<std::string>& args) {
    ByteBuffer identityBytes = patchFs.getFileContents("patch://identity.meta");
    std::string identity = std::string(identityBytes.begin(), identityBytes.end());
    FatePatcher patcher(patchFs, input, output, args);
    if (identity == "extra") {
        spdlog::info("Mode: Extra");
        patcher.patchExtra();
    } else if (identity == "ccc") {
        spdlog::info("Mode: CCC");
        patcher.patchCCC();
    } else {
        bail("Failed to identify mode from PatchFS.");
    }
}
