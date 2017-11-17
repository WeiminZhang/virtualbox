#include <iprt/symlink.h>
#define RTDIRCREATE_FLAGS_NOT_CONTENT_INDEXED_SET           UINT32_C(0)
/** Valid mask. */
#define RTDIRCREATE_FLAGS_VALID_MASK                        UINT32_C(0x00000007)
/** @name RTDIR_F_XXX - RTDirOpenFiltered flags.
#define RTDIR_F_NO_SYMLINKS     RT_BIT_32(0)
/** Deny relative opening of anything above this directory. */
#define RTDIR_F_DENY_ASCENT     RT_BIT_32(1)
/** Valid flag mask.   */
#define RTDIR_F_VALID_MASK      UINT32_C(0x00000003)
 * Opens a directory with flags and optional filtering.
 * @param   fFlags      Open flags, RTDIR_F_XXX.
RTDECL(int) RTDirOpenFiltered(PRTDIR *ppDir, const char *pszPath, RTDIRFILTER enmFilter, uint32_t fFlags);
/**
 * Checks if the supplied directory handle is valid.
 *
 * @returns true if valid.
 * @returns false if invalid.
 * @param   hDir        The directory handle.
 */
RTDECL(bool) RTDirIsValid(PRTDIR hDir);


/** @defgroup grp_rt_dir_rel    Directory relative APIs
 *
 * This group of APIs allows working with paths that are relative to an open
 * directory, therebye eliminating some classic path related race conditions on
 * systems with native support for these kinds of operations.
 *
 * On NT (Windows) there is native support for addressing files, directories and
 * stuff _below_ the open directory.  It is not possible to go upwards
 * (hDir:../../grandparent), at least not with NTFS, forcing us to use the
 * directory path as a fallback and opening us to potential races.
 *
 * On most unix-like systems here is now native support for all of this.
 *
 * @{ */

/**
 * Open a file relative to @a hDir.
 *
 * @returns IPRT status code.
 * @param   hDir            The directory to open relative to.
 * @param   pszRelFilename  The relative path to the file.
 * @param   fOpen           Open flags, i.e a combination of the RTFILE_O_XXX
 *                          defines.  The ACCESS, ACTION and DENY flags are
 *                          mandatory!
 * @param   phFile          Where to store the handle to the opened file.
 *
 * @sa      RTFileOpen
 */
RTDECL(int)  RTDirRelFileOpen(PRTDIR hDir, const char *pszRelFilename, uint64_t fOpen, PRTFILE phFile);



/**
 * Opens a directory relative to @a hDir.
 *
 * @returns IPRT status code.
 * @param   hDir            The directory to open relative to.
 * @param   pszDir          The relative path to the directory to open.
 * @param   phDir           Where to store the directory handle.
 *
 * @sa      RTDirOpen
 */
RTDECL(int) RTDirRelDirOpen(PRTDIR hDir, const char *pszDir, PRTDIR *phDir);

/**
 * Opens a directory relative to @a hDir, with flags and optional filtering.
 *
 * @returns IPRT status code.
 * @param   hDir            The directory to open relative to.
 * @param   pszDirAndFilter The relative path to the directory to search, this
 *                          must include wildcards.
 * @param   enmFilter       The kind of filter to apply. Setting this to
 *                          RTDIRFILTER_NONE makes this function behave like
 *                          RTDirOpen.
 * @param   fFlags          Open flags, RTDIR_F_XXX.
 * @param   phDir           Where to store the directory handle.
 *
 * @sa      RTDirOpenFiltered
 */
RTDECL(int) RTDirRelDirOpenFiltered(PRTDIR hDir, const char *pszDirAndFilter, RTDIRFILTER enmFilter,
                                    uint32_t fFlags, PRTDIR *phDir);

/**
 * Creates a directory relative to @a hDir.
 *
 * @returns IPRT status code.
 * @param   hDir            The directory @a pszRelPath is relative to.
 * @param   pszRelPath      The relative path to the directory to create.
 * @param   fMode           The mode of the new directory.
 * @param   fCreate         Create flags, RTDIRCREATE_FLAGS_XXX.
 * @param   phSubDir        Where to return the handle of the created directory.
 *                          Optional.
 *
 * @sa      RTDirCreate
 */
RTDECL(int) RTDirRelDirCreate(PRTDIR hDir, const char *pszRelPath, RTFMODE fMode, uint32_t fCreate, PRTDIR *phSubDir);

/**
 * Removes a directory relative to @a hDir if empty.
 *
 * @returns IPRT status code.
 * @param   hDir            The directory @a pszRelPath is relative to.
 * @param   pszRelPath      The relative path to the directory to remove.
 *
 * @sa      RTDirRemove
 */
RTDECL(int) RTDirRelDirRemove(PRTDIR hDir, const char *pszRelPath);


/**
 * Query information about a file system object relative to @a hDir.
 *
 * @returns IPRT status code.
 * @retval  VINF_SUCCESS if the object exists, information returned.
 * @retval  VERR_PATH_NOT_FOUND if any but the last component in the specified
 *          path was not found or was not a directory.
 * @retval  VERR_FILE_NOT_FOUND if the object does not exist (but path to the
 *          parent directory exists).
 *
 * @param   hDir            The directory @a pszRelPath is relative to.
 * @param   pszRelPath      The relative path to the file system object.
 * @param   pObjInfo        Object information structure to be filled on successful
 *                          return.
 * @param   enmAddAttr      Which set of additional attributes to request.
 *                          Use RTFSOBJATTRADD_NOTHING if this doesn't matter.
 * @param   fFlags          RTPATH_F_ON_LINK or RTPATH_F_FOLLOW_LINK.
 *
 * @sa      RTPathQueryInfoEx
 */
RTDECL(int) RTDirRelPathQueryInfo(PRTDIR hDir, const char *pszRelPath, PRTFSOBJINFO pObjInfo,
                                  RTFSOBJATTRADD enmAddAttr, uint32_t fFlags);

/**
 * Changes the mode flags of a file system object relative to @a hDir.
 *
 * The API requires at least one of the mode flag sets (Unix/Dos) to
 * be set. The type is ignored.
 *
 * @returns IPRT status code.
 * @param   hDir            The directory @a pszRelPath is relative to.
 * @param   pszRelPath      The relative path to the file system object.
 * @param   fMode           The new file mode, see @ref grp_rt_fs for details.
 * @param   fFlags          RTPATH_F_ON_LINK or RTPATH_F_FOLLOW_LINK.
 *
 * @sa      RTPathSetMode
 */
RTDECL(int) RTDirRelPathSetMode(PRTDIR hDir, const char *pszRelPath, RTFMODE fMode, uint32_t fFlags);

/**
 * Changes one or more of the timestamps associated of file system object
 * relative to @a hDir.
 *
 * @returns IPRT status code.
 * @param   hDir                The directory @a pszRelPath is relative to.
 * @param   pszRelPath          The relative path to the file system object.
 * @param   pAccessTime         Pointer to the new access time.
 * @param   pModificationTime   Pointer to the new modification time.
 * @param   pChangeTime         Pointer to the new change time. NULL if not to be changed.
 * @param   pBirthTime          Pointer to the new time of birth. NULL if not to be changed.
 * @param   fFlags              RTPATH_F_ON_LINK or RTPATH_F_FOLLOW_LINK.
 *
 * @remark  The file system might not implement all these time attributes,
 *          the API will ignore the ones which aren't supported.
 *
 * @remark  The file system might not implement the time resolution
 *          employed by this interface, the time will be chopped to fit.
 *
 * @remark  The file system may update the change time even if it's
 *          not specified.
 *
 * @remark  POSIX can only set Access & Modification and will always set both.
 *
 * @sa      RTPathSetTimesEx
 */
RTDECL(int) RTDirRelPathSetTimes(PRTDIR hDir, const char *pszRelPath, PCRTTIMESPEC pAccessTime, PCRTTIMESPEC pModificationTime,
                                 PCRTTIMESPEC pChangeTime, PCRTTIMESPEC pBirthTime, uint32_t fFlags);

/**
 * Changes the owner and/or group of a file system object relative to @a hDir.
 *
 * @returns IPRT status code.
 * @param   hDir            The directory @a pszRelPath is relative to.
 * @param   pszRelPath      The relative path to the file system object.
 * @param   uid             The new file owner user id.  Pass NIL_RTUID to leave
 *                          this unchanged.
 * @param   gid             The new group id.  Pass NIL_RTGID to leave this
 *                          unchanged.
 * @param   fFlags          RTPATH_F_ON_LINK or RTPATH_F_FOLLOW_LINK.
 *
 * @sa      RTPathSetOwnerEx
 */
RTDECL(int) RTDirRelPathSetOwner(PRTDIR hDir, const char *pszRelPath, uint32_t uid, uint32_t gid, uint32_t fFlags);

/**
 * Renames a directory relative path within a filesystem.
 *
 * This will rename symbolic links.  If RTPATHRENAME_FLAGS_REPLACE is used and
 * pszDst is a symbolic link, it will be replaced and not its target.
 *
 * @returns IPRT status code.
 * @param   hDirSrc         The directory the source path is relative to.
 * @param   pszSrc          The source path, relative to @a hDirSrc.
 * @param   hDirDst         The directory the destination path is relative to.
 * @param   pszDst          The destination path, relative to @a hDirDst.
 * @param   fRename         Rename flags, RTPATHRENAME_FLAGS_XXX.
 *
 * @sa      RTPathRename
 */
RTDECL(int) RTDirRelPathRename(PRTDIR hDirSrc, const char *pszSrc, PRTDIR hDirDst, const char *pszDst, unsigned fRename);

/**
 * Removes the last component of the directory relative path.
 *
 * @returns IPRT status code.
 * @param   hDir            The directory @a pszRelPath is relative to.
 * @param   pszRelPath      The relative path to the file system object.
 * @param   fUnlink         Unlink flags, RTPATHUNLINK_FLAGS_XXX.
 *
 * @sa      RTPathUnlink
 */
RTDECL(int) RTDirRelPathUnlink(PRTDIR hDir, const char *pszRelPath, uint32_t fUnlink);



/**
 * Creates a symbolic link (@a pszSymlink) relative to @a hDir targeting @a
 * pszTarget.
 *
 * @returns IPRT status code.
 * @param   hDir            The directory @a pszSymlink is relative to.
 * @param   pszSymlink      The relative path of the symbolic link.
 * @param   pszTarget       The path to the symbolic link target.  This is
 *                          relative to @a pszSymlink or an absolute path.
 * @param   enmType         The symbolic link type.  For Windows compatability
 *                          it is very important to set this correctly.  When
 *                          RTSYMLINKTYPE_UNKNOWN is used, the API will try
 *                          make a guess and may attempt query information
 *                          about @a pszTarget in the process.
 * @param   fCreate         Create flags, RTSYMLINKCREATE_FLAGS_XXX.
 *
 * @sa      RTSymlinkCreate
 */
RTDECL(int) RTDirRelSymlinkCreate(PRTDIR hDir, const char *pszSymlink, const char *pszTarget,
                                  RTSYMLINKTYPE enmType, uint32_t fCreate);

/**
 * Read the symlink target relative to @a hDir.
 *
 * @returns IPRT status code.
 * @retval  VERR_NOT_SYMLINK if @a pszSymlink does not specify a symbolic link.
 * @retval  VERR_BUFFER_OVERFLOW if the link is larger than @a cbTarget.  The
 *          buffer will contain what all we managed to read, fully terminated
 *          if @a cbTarget > 0.
 *
 * @param   hDir            The directory @a pszSymlink is relative to.
 * @param   pszSymlink      The relative path to the symbolic link that should
 *                          be read.
 * @param   pszTarget       The target buffer.
 * @param   cbTarget        The size of the target buffer.
 * @param   fRead           Read flags, RTSYMLINKREAD_FLAGS_XXX.
 *
 * @sa      RTSymlinkRead
 */
RTDECL(int) RTDirRelSymlinkRead(PRTDIR hDir, const char *pszSymlink, char *pszTarget, size_t cbTarget, uint32_t fRead);

/** @} */

