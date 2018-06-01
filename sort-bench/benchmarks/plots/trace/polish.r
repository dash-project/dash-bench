#!/usr/bin/env Rscript

library(dplyr)

args = commandArgs(trailingOnly=TRUE)

if (length(args) == 0) {
  stop("Usage: <script> <infile>", call.=FALSE)
}

fileName <- basename(args[1])
fileName <- sub("^(.*)(\\.[a-z0-9].*$)", "\\1", fileName)
dirname <- dirname(args[1])

outSvg <- paste(fileName, "-plot.csv", sep="")
outSvg <- paste(dirname, outSvg, sep="/")
outDetail <- paste(fileName, "-detail.csv", sep="")
outDetail <- paste(dirname, outDetail, sep="/")

# read csv
data.raw <- read.csv(args[1], header=TRUE, strip.white=TRUE)
# remove context column
data.raw <- subset(data.raw, select=-context)
# reorder columns
data.raw <- data.raw[c("state","unit","start","end")]

#discard detail traces about partition border finding
data.svg <- data.raw %>% filter(!grepl("[0-9]\\.[0-9].*", state))
write.table(data.svg, outSvg, row.names = F, col.names = T, sep = ",")

# print out the generated svg filename
cat(outSvg, "\n")

# calculate details if available

data.detail <- data.raw %>% filter(grepl("[0-9]\\.[0-9].*", state))

if (nrow(data.detail) > 0) {
    # calculate duration
    data.detail$duration <- (data.detail$end - data.detail$start)
    # grep iteration and push it to a separate column
    data.detail <- mutate(data.detail, iter=gsub("([0-9]\\.[0-9].*Iteration_)([0-9]+)(_.*)", "\\2", data.detail$state))
    # sort by duration
    data.detail <- data.detail %>% arrange(desc(duration))
    # better captions
    data.detail$state <- gsub("(.*_[0-9]+_)(.*)",replacement = "\\2",data.detail$state)

    # summarize if you want this
    data.detail <- data.detail %>% group_by(unit,state) %>% summarize(median=median(duration), min=min(duration), max=max(duration))
    data.detail <- data.detail %>% arrange(desc(max))
    write.table(data.detail, outDetail, row.names = F, col.names = T, sep = ",")
}

