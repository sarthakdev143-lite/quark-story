#!/usr/bin/env node

/**
 * Repo Code Scraper - Extracts all code files from a repository into a single text file
 * Usage: node repo-scraper.js [repository-path] [output-file]
 * Example: node repo-scraper.js . repo-code.txt
 */

const fs = require('fs');
const path = require('path');
const { promisify } = require('util');
const readdir = promisify(fs.readdir);
const stat = promisify(fs.stat);
const readFile = promisify(fs.readFile);
const writeFile = promisify(fs.writeFile);

// Configuration
const CONFIG = {
    // File extensions to include (code files)
    includeExtensions: [
        '.js', '.jsx', '.ts', '.tsx', '.py', '.java', '.c', '.cpp', '.h', '.hpp',
        '.cs', '.go', '.rb', '.php', '.swift', '.kt', '.rs', '.scala', '.pl',
        '.sh', '.bash', '.zsh', '.fish', '.ps1', '.cmd', '.bat',
        '.html', '.htm', '.css', '.scss', '.sass', '.less', '.styl',
        '.json', '.xml', '.yaml', '.yml', '.toml', '.ini', '.cfg', '.conf',
        '.md', '.markdown', '.txt', '.log',
        '.sql', '.db', '.sqlite',
        '.dockerfile', '.dockerignore', '.gitignore', '.env',
        '.vue', '.svelte', '.astro', '.slim', '.erb', '.ejs', '.hbs',
        '.lua', '.r', '.dart', '.elm', '.clj', '.cljs', '.ex', '.exs'
    ],
    
    // Directories to exclude
    excludeDirs: [
        'node_modules', '.git', 'dist', 'build', 'coverage', 
        '.next', '.nuxt', '.cache', 'tmp', 'temp',
        'vendor', 'bower_components', 'jspm_packages',
        '.idea', '.vscode', '.vs', '__pycache__',
        'target', 'out', 'bin', 'obj', 'lib',
        'logs', 'uploads', 'downloads', 'C:vcpkg'
    ],
    
    // Files to exclude (by name)
    excludeFiles: [
        'package-lock.json', 'yarn.lock', 'pnpm-lock.yaml',
        'composer.lock', 'Gemfile.lock', 'Cargo.lock',
        '.DS_Store', 'Thumbs.db',
        '*.min.js', '*.min.css', '*.bundle.js'
    ],
    
    // Maximum file size to process (in bytes) - 5MB
    maxFileSize: 5 * 1024 * 1024,
    
    // Output formatting
    showFilePaths: true,
    addSeparators: true,
    includeGitInfo: false
};

class RepoScraper {
    constructor(repoPath = '.', outputFile = 'repo-code.txt') {
        this.repoPath = path.resolve(repoPath);
        this.outputFile = outputFile;
        this.files = [];
        this.totalSize = 0;
        this.fileCount = 0;
        this.errorCount = 0;
        this.startTime = Date.now();
    }

    /**
     * Check if a directory should be excluded
     */
    shouldExcludeDir(dirName) {
        return CONFIG.excludeDirs.some(exclude => 
            dirName === exclude || dirName.startsWith('.') && exclude === '.*'
        );
    }

    /**
     * Check if a file should be excluded
     */
    shouldExcludeFile(fileName) {
        // Check exact matches
        if (CONFIG.excludeFiles.some(exclude => fileName === exclude)) {
            return true;
        }
        
        // Check glob patterns
        return CONFIG.excludeFiles.some(pattern => {
            if (pattern.includes('*')) {
                const regexPattern = pattern.replace(/\*/g, '.*');
                const regex = new RegExp(`^${regexPattern}$`);
                return regex.test(fileName);
            }
            return false;
        });
    }

    /**
     * Check if a file has an allowed extension
     */
    hasAllowedExtension(fileName) {
        const ext = path.extname(fileName).toLowerCase();
        return CONFIG.includeExtensions.includes(ext);
    }

    /**
     * Recursively scan directory for code files
     */
    async scanDirectory(dirPath) {
        try {
            const entries = await readdir(dirPath, { withFileTypes: true });
            
            for (const entry of entries) {
                const fullPath = path.join(dirPath, entry.name);
                const relativePath = path.relative(this.repoPath, fullPath);
                
                if (entry.isDirectory()) {
                    // Skip excluded directories
                    if (this.shouldExcludeDir(entry.name)) {
                        continue;
                    }
                    await this.scanDirectory(fullPath);
                } else if (entry.isFile()) {
                    // Check if we should include this file
                    if (this.shouldExcludeFile(entry.name)) {
                        continue;
                    }
                    
                    if (!this.hasAllowedExtension(entry.name)) {
                        continue;
                    }
                    
                    try {
                        const stats = await stat(fullPath);
                        if (stats.size <= CONFIG.maxFileSize) {
                            this.files.push({
                                path: fullPath,
                                relativePath: relativePath,
                                name: entry.name,
                                size: stats.size
                            });
                            this.totalSize += stats.size;
                            this.fileCount++;
                        } else {
                            console.warn(`⚠️  Skipping ${relativePath} (${(stats.size / 1024 / 1024).toFixed(2)}MB > ${CONFIG.maxFileSize / 1024 / 1024}MB limit)`);
                        }
                    } catch (err) {
                        console.error(`❌ Error reading ${relativePath}: ${err.message}`);
                        this.errorCount++;
                    }
                }
            }
        } catch (err) {
            console.error(`❌ Error scanning ${dirPath}: ${err.message}`);
            this.errorCount++;
        }
    }

    /**
     * Read file content with proper encoding
     */
    async readFileContent(fileInfo) {
        try {
            // Try to detect binary files by checking if it's a text file
            // Simple check: try to read as UTF-8 and see if it has null bytes
            const buffer = await readFile(fileInfo.path);
            
            // Check if binary (contains null byte)
            if (buffer.includes(0)) {
                // Binary file - skip or base64 encode? Let's skip for now
                console.warn(`⚠️  Skipping binary file: ${fileInfo.relativePath}`);
                return null;
            }
            
            // Try to decode as UTF-8, replace invalid characters
            return buffer.toString('utf-8');
        } catch (err) {
            console.error(`❌ Error reading ${fileInfo.relativePath}: ${err.message}`);
            this.errorCount++;
            return null;
        }
    }

    /**
     * Generate separator line
     */
    generateSeparator(fileInfo) {
        const separator = '='.repeat(80);
        const name = ` ${fileInfo.relativePath} `;
        const padding = Math.max(0, 80 - name.length);
        const leftPadding = Math.floor(padding / 2);
        const rightPadding = padding - leftPadding;
        
        return `${'='.repeat(leftPadding)}${name}${'='.repeat(rightPadding)}`;
    }

    /**
     * Generate header for the output file
     */
    generateHeader() {
        const timestamp = new Date().toISOString();
        const lines = [
            '='.repeat(80),
            `REPO CODE DUMP - Generated on ${timestamp}`,
            `Repository: ${this.repoPath}`,
            `Total files: ${this.fileCount}`,
            `Total size: ${(this.totalSize / 1024 / 1024).toFixed(2)} MB`,
            '='.repeat(80),
            '',
            ''
        ];
        return lines.join('\n');
    }

    /**
     * Generate footer for the output file
     */
    generateFooter() {
        const elapsed = ((Date.now() - this.startTime) / 1000).toFixed(2);
        const lines = [
            '',
            '',
            '='.repeat(80),
            `Scraping completed in ${elapsed}s`,
            `Files processed: ${this.fileCount}`,
            `Errors: ${this.errorCount}`,
            '='.repeat(80)
        ];
        return lines.join('\n');
    }

    /**
     * Main scraping function
     */
    async scrape() {
        console.log(`📂 Scanning repository: ${this.repoPath}`);
        console.log('🔍 Looking for code files...\n');
        
        await this.scanDirectory(this.repoPath);
        
        if (this.fileCount === 0) {
            console.warn('⚠️  No code files found in the repository.');
            return;
        }
        
        console.log(`\n✅ Found ${this.fileCount} code files (${(this.totalSize / 1024 / 1024).toFixed(2)} MB)`);
        console.log('📝 Writing to output file...\n');
        
        // Sort files by path for consistent output
        this.files.sort((a, b) => a.relativePath.localeCompare(b.relativePath));
        
        // Prepare content
        const outputLines = [];
        
        // Add header
        outputLines.push(this.generateHeader());
        
        let processedFiles = 0;
        
        for (const fileInfo of this.files) {
            processedFiles++;
            const progress = ((processedFiles / this.fileCount) * 100).toFixed(1);
            process.stdout.write(`\r⏳ Processing: ${progress}% (${processedFiles}/${this.fileCount})`);
            
            const content = await this.readFileContent(fileInfo);
            if (content === null) continue;
            
            // Add file separator and path
            if (CONFIG.addSeparators) {
                outputLines.push(this.generateSeparator(fileInfo));
            } else if (CONFIG.showFilePaths) {
                outputLines.push(`\n--- File: ${fileInfo.relativePath} ---\n`);
            }
            
            // Add file content
            outputLines.push(content);
            
            // Add a blank line between files for readability
            if (CONFIG.addSeparators) {
                outputLines.push('');
            }
        }
        
        process.stdout.write('\r✅ Scraping complete!   \n\n');
        
        // Add footer
        outputLines.push(this.generateFooter());
        
        // Write to file
        try {
            await writeFile(this.outputFile, outputLines.join('\n'), 'utf-8');
            const outputSize = (await stat(this.outputFile)).size;
            
            console.log(`✅ Successfully wrote to: ${this.outputFile}`);
            console.log(`📊 Output file size: ${(outputSize / 1024 / 1024).toFixed(2)} MB`);
            console.log(`📁 Files processed: ${this.fileCount}`);
            console.log(`⚠️  Errors: ${this.errorCount}`);
            console.log(`⏱️  Time elapsed: ${((Date.now() - this.startTime) / 1000).toFixed(2)}s`);
        } catch (err) {
            console.error(`❌ Error writing output file: ${err.message}`);
            throw err;
        }
    }
}

// CLI Usage
async function main() {
    const args = process.argv.slice(2);
    const repoPath = args[0] || '.';
    const outputFile = args[1] || 'repo-code.txt';
    
    // Check if help is requested
    if (args.includes('--help') || args.includes('-h')) {
        console.log(`
📚 Repo Code Scraper - Extract all code files into a single text file

Usage:
  node repo-scraper.js [repository-path] [output-file]

Arguments:
  repository-path  Path to the repository (default: current directory)
  output-file      Output file name (default: repo-code.txt)

Examples:
  node repo-scraper.js . code-dump.txt
  node repo-scraper.js /path/to/repo repo-files.txt

Configuration:
  Edit the CONFIG object in the script to customize:
  - File extensions to include
  - Directories to exclude
  - File size limits
  - Output formatting
        `);
        return;
    }
    
    try {
        const scraper = new RepoScraper(repoPath, outputFile);
        await scraper.scrape();
    } catch (err) {
        console.error(`❌ Fatal error: ${err.message}`);
        process.exit(1);
    }
}

// Run if executed directly
if (require.main === module) {
    main();
}

module.exports = RepoScraper;